/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file mrubyc_vm.c
 * @brief Implementation of mruby/c virtual machine management
 * @details Implements functions for controlling the mruby/c virtual machine
 */
#include "mrubyc_vm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "../../mrubyc/src/mrubyc.h"
#include "../api/api.h"
#include "../api/ble.h"
#include "../api/blink.h"
#include "../api/input.h"
#include "../api/led.h"
#include "../api/symbol.h"
#include "../drv/ble.h"
#include "../lib/fn.h"
#include "../rb/slot1.h"
#include "../rb/slot2.h"
#include "blink.h"
#include "init.h"

LOG_MODULE_REGISTER(app_mrubyc_vm, LOG_LEVEL_DBG);

/**
 * @brief Size of heap memory for the mruby/c VM in bytes
 */
#define MRBC_HEAP_MEMORY_SIZE (15 * 1024)

/**
 * @brief Stack size for the mruby/c VM main thread in bytes
 */
#define MRUBYC_VM_MAIN_STACK_SIZE (50 * 1024)

/**
 * @brief Flag indicating if VM reload is pending
 */
static bool request_mruby_reload = true;

/**
 * @brief Loads bytecode from storage or default slots
 *
 * @param kSlot The slot to load from
 * @param bytecode Buffer to store the bytecode
 * @param kLength Maximum length of the buffer
 */
static void load_bytecode(const blink_slot_t kSlot, uint8_t *const bytecode,
                          const size_t kLength);

/**
 * @brief Main function for the mruby/c VM thread
 *
 * @param unused1 Unused parameter
 * @param unused2 Unused parameter
 * @param unused3 Unused parameter
 */
static void mrubyc_vm_main(void *, void *, void *);

/**
 * @brief Timer handler for the mruby/c VM
 *
 * @param timer Timer instance
 */
static void mrubyc_timerhandler(struct k_timer *const timer) { mrbc_tick(); }

/**
 * @brief Thread definition for the mruby/c VM main function
 */
K_THREAD_DEFINE(th_mrubyc_vm_main, MRUBYC_VM_MAIN_STACK_SIZE, mrubyc_vm_main,
                NULL, NULL, NULL, 1, 0, 1);

/**
 * @brief Timer definition for the mruby/c VM
 */
K_TIMER_DEFINE(timer_mrubyc, mrubyc_timerhandler, NULL);

/**
 * @brief Sets the reload flag for the mruby/c virtual machine
 *
 * @details When set, the VM will reload bytecode on the next cycle
 *
 * @return fn_t kSuccess if successful
 */
fn_t app_mrubyc_vm_set_reload(void) {
  request_mruby_reload = true;
  return kSuccess;
}

/**
 * @brief Gets the current state of the reload flag
 *
 * @return true if reload is pending
 * @return false if no reload is pending
 */
bool app_mrubyc_vm_get_reload(void) { return request_mruby_reload; }

/**
 * @brief Main function for the mruby/c VM thread
 *
 * @details Initializes the VM, loads bytecode, creates tasks, and runs the VM
 * in a loop
 *
 * @param unused1 Unused parameter
 * @param unused2 Unused parameter
 * @param unused3 Unused parameter
 */
static void mrubyc_vm_main(void *, void *, void *) {
  int64_t timestamp = k_uptime_get();
  char buf_blink_time[100] = {0};

  while (1) {
    mrbc_tcb *tcb[MAX_VM_COUNT] = {NULL};
    uint8_t memory_pool[MRBC_HEAP_MEMORY_SIZE] = {0};
    uint8_t bytecode_slot1[BLINK_MAX_BYTECODE_SIZE] = {0};
    uint8_t bytecode_slot2[BLINK_MAX_BYTECODE_SIZE] = {0};

    // mruby/c initialize
    mrbc_init(memory_pool, MRBC_HEAP_MEMORY_SIZE);

    ////////////////////
    // Symbol
    if (kSuccess != api_symbol_define()) {
      LOG_ERR("Failed to define symbol");
    }
    // Class, Method
    api_led_define();    // LED.*
    api_input_define();  // Input.*
    api_ble_define();    // BLE.*
    api_blink_define();  // Blink.*

    ////////////////////
    // If no reload request, wait for request
    if (false == request_mruby_reload) {
#if CONFIG_DEBUG
      uint8_t i = 99;
      while (false == request_mruby_reload) {
        k_msleep(100);
        i++;
        if (0 == (i % 100)) {
          ble_print("Abnormal end was detected. Waiting for Blink.");
          i = 0;
        }
      }
#else
      ble_print("Abnormal end was detected. Factory reset is in progress.");
      init_factory_reset();
#endif
    }
    // Clear reload request flag
    request_mruby_reload = false;

    ////////////////////
    // Load mruby bytecode
    load_bytecode(kBlinkSlot1, bytecode_slot1,
                  sizeof(bytecode_slot1) / sizeof(bytecode_slot1[0]));
    load_bytecode(kBlinkSlot2, bytecode_slot2,
                  sizeof(bytecode_slot2) / sizeof(bytecode_slot2[0]));

    ////////////////////
    // mruby/c create task
    tcb[0] = mrbc_create_task(bytecode_slot1, NULL);
    tcb[1] = mrbc_create_task(bytecode_slot2, NULL);
    if ((tcb[0] == NULL) || (tcb[1] == NULL)) {
      LOG_ERR("Failed to create task");
    }
    // set priority
    mrbc_change_priority(tcb[0], 1);
    mrbc_change_priority(tcb[1], 2);

    ////////////////////
    snprintf(buf_blink_time, sizeof(buf_blink_time), "Blinked (%lli ms)\n",
             k_uptime_delta(&timestamp));
    ble_print(buf_blink_time);

    k_timer_start(&timer_mrubyc, K_NO_WAIT, K_MSEC(1));
    mrbc_run();
    k_timer_stop(&timer_mrubyc);

    snprintf(buf_blink_time, sizeof(buf_blink_time),
             "mrbc_run Stopped (uptime: %lli ms)\n",
             k_uptime_delta(&timestamp));
    ble_print(buf_blink_time);

    ////////////////////
    // mruby/c cleanup
    mrbc_cleanup();
  }
}

/**
 * @brief Loads bytecode from storage or default slots
 *
 * @details Attempts to load bytecode from non-volatile memory first.
 *          If that fails, loads from factory default program.
 *
 * @param kSlot The slot to load from
 * @param bytecode Buffer to store the bytecode
 * @param kLength Maximum length of the buffer
 */
static void load_bytecode(const blink_slot_t kSlot, uint8_t *const bytecode,
                          const size_t kLength) {
  ssize_t rc = 0;
  rc = blink_get_data_length(kSlot);
  if ((0 < rc) && (kLength >= rc)) {
    // Load from non-volatile memory
    rc = blink_load(kSlot, bytecode, kLength);
    if (0 < rc) {
      LOG_DBG("Slot:%d, Size:%d/%d", kSlot, rc, kLength);
      return;
    }
  } else {
    // Load from factory default program
    switch (kSlot) {
      case kBlinkSlot1:
        rc = (ssize_t)sizeof(slot1);
        if (rc <= (ssize_t)kLength) {
          memcpy(bytecode, &slot1[0], (size_t)rc);
        }
        break;
      case kBlinkSlot2:
        rc = (ssize_t)sizeof(slot2);
        if (rc <= (ssize_t)kLength) {
          memcpy(bytecode, &slot2[0], (size_t)rc);
        }
        break;
      default:
        break;
    }
    if (0 < rc) {
      LOG_DBG("Slot:%d, Size:%d/%d, Factory default program loaded.", kSlot, rc,
              kLength);
    }
  }
}

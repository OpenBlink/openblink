/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file vm.c
 * @brief mruby/c VM lifecycle management
 * @details Implements the VM run loop, restart handling, and the VM lock used
 * by Blink.lock/Blink.unlock. Bytecode and heap buffers are statically
 * allocated so the hosting thread only needs a small stack.
 */
#include "openblink/vm.h"

#include <mrubyc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "internal.h"
#include "openblink/blink.h"

/** @brief Heap memory for the mruby/c VM */
static uint8_t memory_pool[OPENBLINK_VM_HEAP_SIZE];

/** @brief Bytecode buffers, one per slot */
static uint8_t bytecode[OPENBLINK_SLOT_COUNT][OPENBLINK_MAX_BYTECODE_SIZE];

/** @brief Task control blocks, one per slot */
static mrbc_tcb* tcb[OPENBLINK_SLOT_COUNT];

/** @brief Set by openblink_vm_restart() and consumed by openblink_vm_main() */
static volatile bool restart_requested;

/**
 * @brief Loads the bytecode for one slot into its buffer
 *
 * @details Attempts to load from non-volatile storage first; falls back to
 * the platform factory default program.
 *
 * @param slot Slot to load (1..OPENBLINK_SLOT_COUNT)
 * @param buffer Destination buffer
 * @param capacity Capacity of the buffer in bytes
 * @return bool true if bytecode is available in the buffer
 */
static bool load_slot_bytecode(const openblink_slot_t slot,
                               uint8_t* const buffer, const size_t capacity) {
  openblink_length_t stored_len = 0;
  if ((OPENBLINK_STATUS_OK ==
       openblink_slot_get_data_length(slot, &stored_len)) &&
      (0U < stored_len) && ((size_t)stored_len <= capacity)) {
    openblink_length_t read_len = 0;
    if ((OPENBLINK_STATUS_OK ==
         openblink_slot_load(slot, buffer, capacity, &read_len)) &&
        (0U < read_len)) {
      return true;
    }
  }

  openblink_length_t default_len = 0;
  if ((OPENBLINK_STATUS_OK == openblink_hal_load_default_bytecode(
                                  slot, buffer, capacity, &default_len)) &&
      (0U < default_len)) {
    return true;
  }

  return false;
}

void openblink_vm_main(void) {
  while (1) {
    restart_requested = false;
    for (size_t i = 0; OPENBLINK_SLOT_COUNT > i; i++) {
      tcb[i] = NULL;
    }

    // mruby/c initialize (starts the tick timer via mrbc_hal_init())
    mrbc_init(memory_pool, OPENBLINK_VM_HEAP_SIZE);

    // Class, Method
    if (OPENBLINK_STATUS_OK != openblink_hal_define_api()) {
      console_print("ERROR: Failed to define platform API\n");
    }
    if (OPENBLINK_STATUS_OK != openblink_define_blink_class()) {
      console_print("ERROR: Failed to define Blink class\n");
    }

    // Load mruby bytecode and create tasks
    size_t task_count = 0;
    for (size_t i = 0; OPENBLINK_SLOT_COUNT > i; i++) {
      memset(bytecode[i], 0, sizeof(bytecode[i]));
      const openblink_slot_t slot = (openblink_slot_t)(i + 1U);
      if (load_slot_bytecode(slot, bytecode[i], sizeof(bytecode[i]))) {
        tcb[i] = mrbc_create_task(bytecode[i], NULL);
        if (NULL == tcb[i]) {
          console_print("ERROR: Failed to create task\n");
        } else {
          task_count++;
        }
      }
    }

    console_print("Blinked\n");

    if (0U < task_count) {
      mrbc_run();
    } else {
      // No runnable task; wait until a reload is requested.
      while (!restart_requested) {
        mrbc_hal_idle_cpu();
      }
    }

    // mruby/c cleanup
    mrbc_cleanup();
  }
}

openblink_status_t openblink_vm_restart(void) {
  if (!openblink_hal_vm_lock(OPENBLINK_VM_RESTART_TIMEOUT_MS)) {
    return OPENBLINK_STATUS_ERROR;
  }

  restart_requested = true;
  for (size_t i = 0; OPENBLINK_SLOT_COUNT > i; i++) {
    if (NULL != tcb[i]) {
      mrbc_terminate_task(tcb[i]);
      mrbc_delete_task(tcb[i]);
    }
  }

  (void)openblink_hal_vm_unlock();
  return OPENBLINK_STATUS_OK;
}

bool openblink_vm_lock(void) {
  return openblink_hal_vm_lock(OPENBLINK_VM_LOCK_TIMEOUT_MS);
}

bool openblink_vm_unlock(void) { return openblink_hal_vm_unlock(); }

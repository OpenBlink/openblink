/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file comm.c
 * @brief Implementation of communication management
 * @details Implements BLE communication functions and event handling
 */
#include "comm.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "../drv/ble.h"
#include "../drv/ble_blink.h"
#include "../lib/fn.h"
#include "blink.h"
#include "init.h"
#include "mrubyc_vm.h"

LOG_MODULE_REGISTER(app_comm, LOG_LEVEL_DBG);

/** @brief Flag indicating if BLE advertising is active */
static volatile bool advertising = false;

/** @brief Flag indicating if BLE is connected to a device */
static volatile bool connected = false;

/**
 * @brief BLE event callback function
 *
 * @param param BLE event parameters
 * @return int 0 on success, negative on error
 */
static int ble_event_cb(BLE_PARAM* param) {
  int err = 0;
  switch (param->event) {
    case BLE_EVENT_INITIALIZED:
      LOG_DBG("COMM: Initialized");
      break;

    case BLE_EVENT_CONNECTED:
      LOG_DBG("COMM: Connected");
      connected = true;
      advertising = false;
      break;

    case BLE_EVENT_DISCONNECTED:
      LOG_DBG("COMM: Disconnected (%d)", param->disconnected.reason);
      connected = false;
      advertising = true;
      break;

    case BLE_EVENT_RECEIVED:
      break;

    case BLE_EVENT_SENT:
      break;

    case BLE_EVENT_BLINK:
      uint8_t* blink_bytecode = (uint8_t*)param->blink.blink_bytecode;
      LOG_DBG("COMM: Blink ... Slot:%d Size:%d", param->blink.slot,
              param->blink.length);

      ssize_t size = blink_store((blink_slot_t)(param->blink.slot),
                                 blink_bytecode, param->blink.length);
      // size:0  NoChange
      // size:>0 Success
      // size:-1 Error
      if (size >= 0) {
        LOG_DBG("COMM: Success");
      } else {
        LOG_ERR("COMM: Blink Store Error %d", size);
        err = -1;
      }
      break;

    case BLE_EVENT_STATUS:
      param->status.mtu = ble_get_mtu();
      break;

    case BLE_EVENT_RELOAD:
      LOG_DBG("COMM:Reloading ...");
      app_mrubyc_vm_restart();
      break;

    case BLE_EVENT_REBOOT:
      LOG_DBG("COMM:Rebooting ...");
      init_reboot();
      break;

    default:
      LOG_ERR("COMM: Unknown event %d", param->event);
      break;
  }
  return err;
}

/**
 * @brief Initializes the communication subsystem
 *
 * @details Sets up BLE, configures the device name, and starts advertising
 *
 * @return fn_t kSuccess if successful, kFailure otherwise
 */
fn_t comm_init(void) {
  fn_t ret = kSuccess;
  char device_name[BLINK_DEVICE_NAME_SIZE] = {0};

  int err = ble_init(ble_event_cb);
  if (err) {
    LOG_ERR("COMM: BLE initialization failed (err %d)", err);
    return kFailure;
  }

  // Set Device Name
  blink_get_name(device_name, sizeof(device_name));
  bt_set_name(device_name);

  // Start Advertising
  const char* name = bt_get_name();
  LOG_DBG("COMM: Start advertising (%s)", name);
  err = ble_start_advertising(name);
  if (err) {
    LOG_ERR("COMM: Failed to start advertising (err %d)", err);
    return kFailure;
  }
  advertising = true;

  return ret;
}

/**
 * @brief Checks if BLE advertising is active
 *
 * @return true if advertising is active
 * @return false if advertising is not active
 */
bool comm_get_advertising(void) { return advertising; }

/**
 * @brief Checks if BLE is connected to a device
 *
 * @return true if connected
 * @return false if not connected
 */
bool comm_get_connected(void) { return connected; }

/**
 * @brief Disconnects the current BLE connection
 *
 * @return fn_t kSuccess if successful, kFailure otherwise
 */
fn_t comm_disconnect(void) {
  LOG_DBG("COMM: Disconnecting...");
  ble_print("Disconnecting...");
  fn_t ret = (ble_disconnect() == 0) ? kSuccess : kFailure;
  return ret;
}

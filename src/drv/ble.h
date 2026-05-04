/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file ble.h
 * @brief Bluetooth Low Energy driver interface
 * @details Provides functions for BLE initialization, connection management,
 * and data transfer
 */
#ifndef DRV_BLE_H
#define DRV_BLE_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <zephyr/types.h>

#include "ble_blink.h"

/**
 * @brief BLE event types for callback notifications
 */
enum {
  BLE_EVENT_INITIALIZED,  /**< BLE stack has been initialized */
  BLE_EVENT_CONNECTED,    /**< BLE connection established */
  BLE_EVENT_DISCONNECTED, /**< BLE connection terminated */
  BLE_EVENT_RECEIVED,     /**< Data received over BLE */
  BLE_EVENT_SENT,         /**< Data sent over BLE */
  BLE_EVENT_BLINK,        /**< Blink bytecode received */
  BLE_EVENT_STATUS,       /**< Status information requested */
  BLE_EVENT_REBOOT,       /**< Reboot request received */
  BLE_EVENT_RELOAD,       /**< Reload request received */
};

/**
 * @brief BLE callback event parameter structure
 * @details Contains event type and associated data for BLE events
 */
#pragma pack(1)
typedef struct {
  uint8_t event; /**< Event type from BLE event enum */
  union {
    struct {
    } initialized; /**< Initialization event data (empty) */
    struct {
    } connected; /**< Connection event data (empty) */
    struct {
      uint8_t reason; /**< Reason for disconnection */
    } disconnected;
    struct {
      uint8_t *data; /**< Pointer to received data */
      size_t size;   /**< Size of received data */
    } received;
    struct {
    } sent; /**< Data sent event data (empty) */
    struct {
      int slot;                /**< Slot for bytecode storage */
      uint8_t *blink_bytecode; /**< Pointer to bytecode data */
      size_t length;           /**< Length of bytecode */
    } blink;
    struct {
      uint16_t mtu; /**< Maximum Transmission Unit */
    } status;
    struct {
    } reboot; /**< Reboot event data (empty) */
    struct {
    } reload; /**< Reload event data (empty) */
  };
} BLE_PARAM;
#pragma pack()

/**
 * @brief BLE callback function type
 * @param param Pointer to BLE event parameters
 * @return int 0 on success, negative on error
 */
typedef int (*BLE_CALLBACK)(BLE_PARAM *param);

/**
 * @brief BLE context structure
 * @details Stores connection and callback information
 */
typedef struct {
  struct bt_conn *conn;  /**< Bluetooth connection handle */
  BLE_CALLBACK event_cb; /**< Event callback function */
} BLE_CONTEXT;

/** @brief Global BLE context (defined in ble.c) */
extern BLE_CONTEXT ble_context;

/**
 * @brief Initializes the BLE subsystem
 *
 * @param cb Callback function for BLE events
 * @return int 0 on success, negative on error
 */
int ble_init(BLE_CALLBACK cb);

/**
 * @brief Disconnects the current BLE connection
 *
 * @return int 0 on success, negative on error
 */
int ble_disconnect();

/**
 * @brief Starts BLE advertising with the specified device name
 *
 * @param local_name Device name to advertise
 * @return int 0 on success, negative on error
 */
int ble_start_advertising(const char *local_name);

/**
 * @brief Stops BLE advertising
 *
 * @return int 0 on success, negative on error
 */
int ble_stop_advertising();

/**
 * @brief Gets the current Maximum Transmission Unit (MTU)
 *
 * @return uint16_t Current MTU size in bytes
 */
uint16_t ble_get_mtu();

/**
 * @brief Gets the current BLE state
 *
 * @details Derives state from Zephyr standard APIs
 *
 * @return int 0=Off, 1=Advertising, 2=Connected
 */
int ble_get_state(void);

#endif  // DRV_BLE_H

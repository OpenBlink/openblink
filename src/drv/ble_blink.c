/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file ble_blink.c
 * @brief Implementation of Bluetooth Low Energy Blink service
 * @details Implements the Blink service for transferring bytecode over BLE
 */
#include "ble_blink.h"

#include <assert.h>
#include <errno.h>
#include <soc.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "../app/blink.h"
#include "ble.h"

LOG_MODULE_REGISTER(ble_blink, LOG_LEVEL_DBG);

/** @brief OpenBlink service UUID declaration */
#define BT_UUID_OPENBLINK_SERVICE BT_UUID_DECLARE_128(OPENBLINK_SERVICE_UUID)

/** @brief Program characteristic UUID for bytecode transfer */
#define OPEN_BLINK_PROGRAM_CHARACTERISTIC_UUID \
  BT_UUID_128_ENCODE(0xad9fdd56, 0x1135, 0x4a84, 0x923c, 0xce5a244385e7)
/** @brief Program characteristic UUID declaration */
#define BT_UUID_OPEN_BLINK_PROGRAM_CHARACTERISTIC_UUID \
  BT_UUID_DECLARE_128(OPEN_BLINK_PROGRAM_CHARACTERISTIC_UUID)

/** @brief Console characteristic UUID for debug output */
#define OPEN_BLINK_CONSOLE_CHARACTERISTIC_UUID \
  BT_UUID_128_ENCODE(0xa015b3de, 0x185a, 0x4252, 0xaa04, 0x7a87d38ce148)
/** @brief Console characteristic UUID declaration */
#define BT_UUID_OPEN_BLINK_CONSOLE_CHARACTERISTIC_UUID \
  BT_UUID_DECLARE_128(OPEN_BLINK_CONSOLE_CHARACTERISTIC_UUID)

/** @brief Status characteristic UUID for device status information */
#define OPEN_BLINK_STATUS_CHARACTERISTIC_UUID \
  BT_UUID_128_ENCODE(0xca141151, 0x3113, 0x448b, 0xb21a, 0x6a6203d253ff)
/** @brief Status characteristic UUID declaration */
#define BT_UUID_OPEN_BLINK_STATUS_CHARACTERISTIC_UUID \
  BT_UUID_DECLARE_128(OPEN_BLINK_STATUS_CHARACTERISTIC_UUID)

/** @brief Blink protocol version */
#define BLINK_VERSION 0x01

/** @brief Command code for data chunk transfer */
#define BLINK_CMD_DATA 'D'  // Data
/** @brief Command code for program execution */
#define BLINK_CMD_PROG 'P'  // Program
/** @brief Command code for device reset */
#define BLINK_CMD_RESET 'R'  // softReset
/** @brief Command code for bytecode reload */
#define BLINK_CMD_RELOAD 'L'  // reLoad

/**
 * @brief Header structure for all Blink protocol chunks
 */
#pragma pack(1)
typedef struct {
  uint8_t version;    /**< Blink protocol version (0x01) */
  uint8_t command;    /**< Command type: 'D':Data, 'P':Program, 'R':Reset,
                         'L':Reload */
} BLINK_CHUNK_HEADER; /**< 2 bytes total */
#pragma pack()

/**
 * @brief Structure for data chunk transfers
 */
#pragma pack(1)
typedef struct {
  BLINK_CHUNK_HEADER header; /**< Common header */
  uint16_t offset;           /**< Offset in bytecode buffer */
  uint16_t size;             /**< Size of data chunk */
} BLINK_CHUNK_DATA;          /**< 6 bytes total */
#pragma pack()

/**
 * @brief Structure for program execution command
 */
#pragma pack(1)
typedef struct {
  BLINK_CHUNK_HEADER header; /**< Common header */
  uint16_t length;           /**< Total bytecode length */
  uint16_t crc;              /**< CRC16 checksum */
  uint8_t slot;              /**< Target slot for bytecode */
  uint8_t reserved;          /**< Reserved for future use */
} BLINK_CHUNK_PROGRAM;       /**< 8 bytes total */
#pragma pack()

// -------------------------------------------------------------------------------------------

/** @brief External reference to BLE context */
extern BLE_CONTEXT ble_context;

/** @brief Buffer for storing received bytecode */
static uint8_t blink_bytecode[BLINK_MAX_BYTECODE_SIZE] = {0};

/**
 * @brief Sends a notification through the program characteristic
 *
 * @param data String to send as notification
 * @return int 0 on success, negative on error
 */
static int notify_blink_program(const char *data);

/**
 * @brief Sends an error message as a notification
 *
 * @param msg Error message to send
 */
static void blink_result_error(const char *msg) {
  LOG_ERR("%s", msg);
  notify_blink_program(msg);
}

/**
 * @brief Processes a data chunk command (BLINK_CMD_DATA)
 *
 * @param header Pointer to the command header
 * @param len Total length of the received data
 * @return int 0 on success, negative on error
 */
static int blink_program_command_D(BLINK_CHUNK_HEADER *header, uint16_t len) {
  BLINK_CHUNK_DATA *data_chunk = (BLINK_CHUNK_DATA *)header;

  const int size = data_chunk->size;
  const int offset = data_chunk->offset;

  if (sizeof(BLINK_CHUNK_DATA) + size != len) {
    LOG_ERR("%d / %d", sizeof(BLINK_CHUNK_DATA) + size, len);
    blink_result_error("ERROR: Blink data size error");
    return -1;
  }

  LOG_DBG(
      "BLE: Blink 'D'ata chunk:"
      "len:%d size:%d offset:%d",
      len, size, offset);

  if (offset + size > BLINK_MAX_BYTECODE_SIZE) {
    blink_result_error("ERROR: Size exceeds buffer limits");
    return -1;
  }

  uint8_t *buffer = (uint8_t *)(data_chunk + 1);
  memcpy(&blink_bytecode[offset], buffer, size);
  return 0;
}

/**
 * @brief Processes a program execution command (BLINK_CMD_PROG)
 *
 * @param header Pointer to the command header
 * @return int 0 on success, negative on error
 */
static int blink_program_command_P(BLINK_CHUNK_HEADER *header) {
  BLINK_CHUNK_PROGRAM *p = (BLINK_CHUNK_PROGRAM *)header;

  LOG_DBG("BLE: Blink 'P'rogram size:%d slot:%d CRC16:0x%08X", p->length,
          p->slot, p->crc);

  if (p->length > BLINK_MAX_BYTECODE_SIZE) {
    blink_result_error("ERROR: Size exceeds buffer limits");
    return -EINVAL;
  }

  // CRC16
  uint16_t crc16 = crc16_reflect(0xd175U, 0xFFFFU, blink_bytecode, p->length);
  LOG_DBG("BLE: Blink CRC16: 0x%08X == 0x%08X", crc16, p->crc);

  if (crc16 == p->crc) {
    BLE_PARAM param = {
        .event = BLE_EVENT_BLINK,
        .blink.blink_bytecode = &blink_bytecode[0],
        .blink.slot = p->slot,
        .blink.length = p->length,
    };

    int err = ble_context.event_cb(&param);
    if (err == 0) {
      LOG_DBG("blink_bytecode:%d", p->length);
      char str[64];
      snprintf(str, sizeof(str), "OK slot:%d", p->slot);
      notify_blink_program(str);
    } else {
      blink_result_error("ERROR: Blink program error");
    }

  } else {
    blink_result_error("ERROR: CRC mismatch");
  }

  // Clear the buffer
  memset(&blink_bytecode, 0, sizeof(blink_bytecode));
  return 0;
}

/**
 * @brief Callback for program characteristic write operations
 *
 * @param conn Bluetooth connection handle
 * @param attr GATT attribute being written to
 * @param buf Buffer containing the data to write
 * @param len Length of the data
 * @param offset Offset to start writing at
 * @param flags Write operation flags
 * @return ssize_t Number of bytes written
 */
static ssize_t blink_write_program(struct bt_conn *conn,
                                   const struct bt_gatt_attr *attr,
                                   const void *buf, uint16_t len,
                                   uint16_t offset, uint8_t flags) {
  ARG_UNUSED(attr);
  ARG_UNUSED(offset);
  ARG_UNUSED(flags);

  BLINK_CHUNK_HEADER *header = (BLINK_CHUNK_HEADER *)buf;
  LOG_DBG("BLE: Blink Command [%c]", header->command);

  // Check the version
  if (header->version != BLINK_VERSION) {
    blink_result_error("ERROR: Blink version mismatch");
  }

  switch (header->command) {
    case BLINK_CMD_DATA:
      blink_program_command_D(header, len);
      break;
    case BLINK_CMD_PROG:
      if (sizeof(BLINK_CHUNK_PROGRAM) != len) {
        blink_result_error("ERROR: Blink size mismatch");
      } else {
        blink_program_command_P(header);
      }
      break;
    case BLINK_CMD_RESET:
      BLE_PARAM param_reset = {
          .event = BLE_EVENT_REBOOT,
      };
      ble_context.event_cb(&param_reset);
      break;
    case BLINK_CMD_RELOAD:
      BLE_PARAM param_reload = {
          .event = BLE_EVENT_RELOAD,
      };
      ble_context.event_cb(&param_reload);
      break;
    default:
      blink_result_error("ERROR: Blink unknown type");
  }

  return len;
}

/**
 * @brief Callback for status characteristic read operations
 *
 * @param conn Bluetooth connection handle
 * @param attr GATT attribute being read from
 * @param buf Buffer to store the read data
 * @param len Maximum length of the buffer
 * @param offset Offset to start reading from
 * @return ssize_t Number of bytes read
 */
static ssize_t blink_read_mtu(struct bt_conn *conn,
                              const struct bt_gatt_attr *attr, void *buf,
                              uint16_t len, uint16_t offset) {
  BLE_PARAM param = {
      .event = BLE_EVENT_STATUS,
      .status.mtu = bt_gatt_get_mtu(conn),
  };
  ble_context.event_cb(&param);

  return bt_gatt_attr_read(conn, attr, buf, len, offset, &param.status.mtu,
                           sizeof(param.status.mtu));
}

/**
 * @brief Callback for Client Characteristic Configuration Descriptor changes
 *
 * @param attr GATT attribute
 * @param value New CCCD value
 */
static void on_cccd_changed(const struct bt_gatt_attr *attr, uint16_t value) {
  LOG_DBG("BLE: CCCD changed to %04x", value);
}

/**
 * @brief BLE GATT attributes for the OpenBlink service
 */
static struct bt_gatt_attr attrs[] = {
    // 0
    BT_GATT_PRIMARY_SERVICE(BT_UUID_OPENBLINK_SERVICE),
    // Console: 1, [2], 3
    BT_GATT_CHARACTERISTIC(BT_UUID_OPEN_BLINK_CONSOLE_CHARACTERISTIC_UUID,
                           BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL,
                           NULL),
    BT_GATT_CCC(on_cccd_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    // Program: 4, [5]
    BT_GATT_CHARACTERISTIC(BT_UUID_OPEN_BLINK_PROGRAM_CHARACTERISTIC_UUID,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, blink_write_program, NULL),
    // MTU: 6, [7]
    BT_GATT_CHARACTERISTIC(BT_UUID_OPEN_BLINK_STATUS_CHARACTERISTIC_UUID,
                           BT_GATT_CHRC_READ, BT_GATT_PERM_READ, blink_read_mtu,
                           NULL, NULL),
};

/** @brief Index of console characteristic in the attributes array */
#define SERVICE_BLINK_CONSOLE 2  // [2]
/** @brief Index of program characteristic in the attributes array */
#define SERVICE_BLINK_PROGRAM 5  // [5]
/** @brief Index of status characteristic in the attributes array */
#define SERVICE_BLINK_STATUS 7  // [7]

/** @brief GATT service definition */
static struct bt_gatt_service service = BT_GATT_SERVICE(attrs);

/**
 * @brief Sends a notification through the program characteristic
 *
 * @param data String to send as notification
 * @return int 0 on success, negative on error
 */
static int notify_blink_program(const char *data) {
  struct bt_conn *conn = ble_context.conn;
  if (conn == NULL) {
    return 0;
  }

  const struct bt_gatt_attr *attr = &attrs[SERVICE_BLINK_PROGRAM];
  if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
    return 0;
  }

  size_t size = strlen(data);
  int err = bt_gatt_notify(conn, attr, data, size);
  if (err) {
    LOG_ERR("BLE: unable to send notification");
  }
  return err;
}

/**
 * @brief Initializes the BLE Blink service
 *
 * @details Registers the GATT service for bytecode transfer and device control
 *
 * @return int 0 on success, negative on error
 */
int ble_blink_init() {
  int err = bt_gatt_service_register(&service);
  if (err) {
    LOG_ERR("BLE: Blink service register error %d", err);
  }
  return err;
}

/**
 * @brief Sends a string over BLE console characteristic
 *
 * @param data String to send
 * @return int 0 on success, negative on error
 */
int ble_print(const char *data) {
  struct bt_conn *conn = ble_context.conn;
  if (conn == NULL) {
    return 0;
  }

  const struct bt_gatt_attr *attr = &attrs[SERVICE_BLINK_CONSOLE];
  if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
    return 0;
  }

  size_t size = strlen(data);
  int err = bt_gatt_notify(conn, attr, data, size);
  if (err) {
    LOG_ERR("BLE: unable to send notification");
  }
  return err;
}

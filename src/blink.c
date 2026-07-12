/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file blink.c
 * @brief Blink protocol parser and bytecode slot management
 * @details Implements the transport-independent Blink protocol ('D', 'P',
 * 'R', 'L') including CRC16 verification, and the slot API that validates
 * arguments before delegating to the platform storage HAL.
 */
#include "openblink/blink.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "openblink/vm.h"

/** @brief Blink protocol version */
#define BLINK_VERSION 0x01U

/** @brief Command code for data chunk transfer */
#define BLINK_CMD_DATA 'D'
/** @brief Command code for program execution */
#define BLINK_CMD_PROG 'P'
/** @brief Command code for device reset */
#define BLINK_CMD_RESET 'R'
/** @brief Command code for bytecode reload */
#define BLINK_CMD_RELOAD 'L'

/** @brief Size of the common frame header ({version, command}) in bytes */
#define BLINK_HEADER_SIZE ((openblink_length_t)2U)
/** @brief Size of a 'D' frame header ({header, offset, size}) in bytes */
#define BLINK_DATA_HEADER_SIZE ((openblink_length_t)6U)
/** @brief Size of a 'P' frame ({header, length, crc, slot, reserved}) */
#define BLINK_PROGRAM_FRAME_SIZE ((openblink_length_t)8U)

/** @brief Buffer for the bytecode being received via 'D' frames */
static uint8_t receive_buffer[OPENBLINK_MAX_BYTECODE_SIZE];

/**
 * @brief Checks whether a slot number is valid
 *
 * @param slot Slot number to check
 * @return bool true if the slot is within 1..OPENBLINK_SLOT_COUNT
 */
static bool slot_is_valid(const openblink_slot_t slot) {
  return (1U <= slot) && ((openblink_slot_t)OPENBLINK_SLOT_COUNT >= slot);
}

/**
 * @brief Reads a 16-bit little-endian value from a byte sequence
 *
 * @param bytes Pointer to the first byte
 * @return uint16_t Decoded value
 */
static uint16_t read_le16(const uint8_t* const bytes) {
  return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

/**
 * @brief Calculates the CRC16 of a byte sequence
 *
 * @details Bit-reflected CRC16 with polynomial 0xD175 (reflected form) and
 * seed 0xFFFF, matching the checksum used by the OpenBlink IDEs.
 *
 * @param data Pointer to the bytes
 * @param len Number of bytes
 * @return uint16_t CRC16 value
 */
static uint16_t crc16(const uint8_t* const data, const size_t len) {
  uint16_t crc = 0xFFFFU;
  for (size_t i = 0; len > i; i++) {
    crc ^= (uint16_t)data[i];
    for (uint8_t bit = 0; 8U > bit; bit++) {
      if (0U != (crc & 0x0001U)) {
        crc = (uint16_t)((crc >> 1) ^ 0xD175U);
      } else {
        crc = (uint16_t)(crc >> 1);
      }
    }
  }
  return crc;
}

/**
 * @brief Sends a NUL-terminated response string to the host
 *
 * @param msg Response string
 */
static void send_response_string(const char* const msg) {
  (void)openblink_hal_send_response(msg, (openblink_length_t)strlen(msg));
}

/**
 * @brief Processes a data chunk frame (BLINK_CMD_DATA)
 *
 * @param frame Pointer to the frame bytes
 * @param len Length of the frame in bytes
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
static openblink_status_t receive_command_data(const uint8_t* const frame,
                                               const openblink_length_t len) {
  if (BLINK_DATA_HEADER_SIZE > len) {
    send_response_string("ERROR: Blink data size error");
    return OPENBLINK_STATUS_ERROR;
  }

  const uint16_t offset = read_le16(&frame[2]);
  const uint16_t size = read_le16(&frame[4]);

  if ((openblink_length_t)(BLINK_DATA_HEADER_SIZE + size) != len) {
    send_response_string("ERROR: Blink data size error");
    return OPENBLINK_STATUS_ERROR;
  }

  if (((uint32_t)offset + (uint32_t)size) >
      (uint32_t)OPENBLINK_MAX_BYTECODE_SIZE) {
    send_response_string("ERROR: Size exceeds buffer limits");
    return OPENBLINK_STATUS_ERROR;
  }

  memcpy(&receive_buffer[offset], &frame[BLINK_DATA_HEADER_SIZE], size);
  return OPENBLINK_STATUS_OK;
}

/**
 * @brief Processes a program execution frame (BLINK_CMD_PROG)
 *
 * @param frame Pointer to the frame bytes
 * @param len Length of the frame in bytes
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
static openblink_status_t receive_command_program(
    const uint8_t* const frame, const openblink_length_t len) {
  if (BLINK_PROGRAM_FRAME_SIZE != len) {
    send_response_string("ERROR: Blink size mismatch");
    return OPENBLINK_STATUS_ERROR;
  }

  const uint16_t length = read_le16(&frame[2]);
  const uint16_t crc = read_le16(&frame[4]);
  const openblink_slot_t slot = (openblink_slot_t)frame[6];

  if ((uint32_t)length > (uint32_t)OPENBLINK_MAX_BYTECODE_SIZE) {
    send_response_string("ERROR: Size exceeds buffer limits");
    return OPENBLINK_STATUS_ERROR;
  }

  if (!slot_is_valid(slot)) {
    send_response_string("ERROR: Invalid slot");
    memset(receive_buffer, 0, sizeof(receive_buffer));
    return OPENBLINK_STATUS_INVALID_ARG;
  }

  openblink_status_t status = OPENBLINK_STATUS_ERROR;
  if (crc16(receive_buffer, (size_t)length) == crc) {
    status = openblink_slot_store(slot, receive_buffer,
                                  (openblink_length_t)length, NULL);
    if (OPENBLINK_STATUS_OK == status) {
      char msg[32];
      (void)snprintf(msg, sizeof(msg), "OK slot:%u", (unsigned int)slot);
      send_response_string(msg);
    } else {
      send_response_string("ERROR: Blink program error");
    }
  } else {
    send_response_string("ERROR: CRC mismatch");
  }

  memset(receive_buffer, 0, sizeof(receive_buffer));
  return status;
}

openblink_status_t openblink_receive(const void* const data,
                                     const openblink_length_t len) {
  const uint8_t* const frame = (const uint8_t*)data;

  if ((NULL == frame) || (BLINK_HEADER_SIZE > len)) {
    send_response_string("ERROR: Blink size mismatch");
    return OPENBLINK_STATUS_ERROR;
  }

  if (BLINK_VERSION != frame[0]) {
    send_response_string("ERROR: Blink version mismatch");
    return OPENBLINK_STATUS_ERROR;
  }

  openblink_status_t status = OPENBLINK_STATUS_OK;
  switch (frame[1]) {
    case BLINK_CMD_DATA:
      status = receive_command_data(frame, len);
      break;
    case BLINK_CMD_PROG:
      status = receive_command_program(frame, len);
      break;
    case BLINK_CMD_RESET:
      status = openblink_hal_reboot();
      break;
    case BLINK_CMD_RELOAD:
      status = openblink_vm_restart();
      break;
    default:
      send_response_string("ERROR: Blink unknown type");
      status = OPENBLINK_STATUS_ERROR;
      break;
  }
  return status;
}

openblink_status_t openblink_slot_store(const openblink_slot_t slot,
                                        const void* const data,
                                        const openblink_length_t len,
                                        openblink_length_t* const written_len) {
  if ((!slot_is_valid(slot)) || (NULL == data) ||
      ((openblink_length_t)OPENBLINK_MAX_BYTECODE_SIZE < len)) {
    return OPENBLINK_STATUS_INVALID_ARG;
  }

  openblink_length_t written = 0;
  const openblink_status_t status =
      openblink_hal_storage_write(slot, data, len, &written);
  if (NULL != written_len) {
    *written_len = written;
  }
  return status;
}

openblink_status_t openblink_slot_load(const openblink_slot_t slot,
                                       void* const data, const size_t capacity,
                                       openblink_length_t* const read_len) {
  if ((!slot_is_valid(slot)) || (NULL == data)) {
    return OPENBLINK_STATUS_INVALID_ARG;
  }

  openblink_length_t read = 0;
  const openblink_status_t status =
      openblink_hal_storage_read(slot, data, capacity, &read);
  if (NULL != read_len) {
    *read_len = read;
  }
  return status;
}

openblink_status_t openblink_slot_get_data_length(
    const openblink_slot_t slot, openblink_length_t* const len) {
  if ((!slot_is_valid(slot)) || (NULL == len)) {
    return OPENBLINK_STATUS_INVALID_ARG;
  }
  return openblink_hal_storage_get_length(slot, len);
}

openblink_status_t openblink_slot_delete(const openblink_slot_t slot) {
  if (!slot_is_valid(slot)) {
    return OPENBLINK_STATUS_INVALID_ARG;
  }
  return openblink_hal_storage_delete(slot);
}

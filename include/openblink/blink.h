/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file blink.h
 * @brief Blink bytecode management
 * @details Manages bytecode slots for the mruby/c virtual machine
 */
#ifndef APP_BLINK_H
#define APP_BLINK_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

/**
 * @brief Maximum size of bytecode that can be stored
 */
#define BLINK_MAX_BYTECODE_SIZE (4016)

/**
 * @brief Size of the device name buffer including BT device name, separator,
 * and ID
 */
#define BLINK_DEVICE_NAME_SIZE (sizeof(CONFIG_BT_DEVICE_NAME) + 1 + 4 + 1)

/**
 * @typedef blink_slot_t
 * @brief Enumeration of bytecode storage slots
 */
typedef enum {
  kBlinkSlot1 = 1U, /**< First bytecode slot */
  kBlinkSlot2 = 2U, /**< Second bytecode slot */
} blink_slot_t;

/**
 * @brief Gets the device name with unique identifier
 *
 * @param name Buffer to store the device name
 * @param kBufSize Size of the buffer
 */
void blink_get_name(char *const name, const size_t kBufSize);

/**
 * @brief Loads bytecode from the specified slot
 *
 * @param kSlot The slot to load from
 * @param data Buffer to store the bytecode
 * @param kLength Maximum length of the buffer
 * @return ssize_t The number of bytes read, or negative on error
 */
ssize_t blink_load(const blink_slot_t kSlot, void *const data,
                   const size_t kLength);

/**
 * @brief Stores bytecode to the specified slot
 *
 * @param kSlot The slot to store to
 * @param kData Pointer to the bytecode data
 * @param kLength Length of the bytecode data
 * @return ssize_t The number of bytes written, or negative on error
 */
ssize_t blink_store(const blink_slot_t kSlot, const void *const kData,
                    const size_t kLength);

/**
 * @brief Gets the length of bytecode in the specified slot
 *
 * @param kSlot The slot to check
 * @return ssize_t The length of the bytecode, or negative on error
 */
ssize_t blink_get_data_length(const blink_slot_t kSlot);

/**
 * @brief Deletes bytecode from the specified slot
 *
 * @param kSlot The slot to delete
 * @return int 0 on success, negative on error
 */
int blink_delete(const blink_slot_t kSlot);

#endif

/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file blink.h
 * @brief Blink protocol receive entry point and bytecode slot management
 * @details Declares the transport-independent receive entry point, the slot
 * API backed by non-volatile storage, and the HAL functions that a platform
 * must implement to support them.
 */
#ifndef OPENBLINK_BLINK_H
#define OPENBLINK_BLINK_H

#include <stddef.h>

#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief First bytecode slot */
#define OPENBLINK_SLOT_1 ((openblink_slot_t)1U)
/** @brief Second bytecode slot */
#define OPENBLINK_SLOT_2 ((openblink_slot_t)2U)

/* ---- Core API ----------------------------------------------------------- */

/**
 * @brief Processes one received Blink protocol frame
 *
 * @details Parses a single frame ('D', 'P', 'R' or 'L') and performs the
 * corresponding action. Responses are emitted through
 * openblink_hal_send_response(). One frame must be delivered per call;
 * framing is the responsibility of the transport. This function is not
 * re-entrant and must be called serially from a single context.
 *
 * @param data Pointer to the frame bytes
 * @param len Length of the frame in bytes
 * @return openblink_status_t OPENBLINK_STATUS_OK if the frame was processed
 * successfully
 */
openblink_status_t openblink_receive(const void* data, openblink_length_t len);

/**
 * @brief Stores bytecode into the specified slot
 *
 * @param slot Target slot (1..OPENBLINK_SLOT_COUNT)
 * @param data Pointer to the bytecode
 * @param len Length of the bytecode in bytes
 * @param written_len Optional output for the number of bytes written (may be
 * NULL)
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_slot_store(openblink_slot_t slot, const void* data,
                                        openblink_length_t len,
                                        openblink_length_t* written_len);

/**
 * @brief Loads bytecode from the specified slot
 *
 * @param slot Source slot (1..OPENBLINK_SLOT_COUNT)
 * @param data Buffer for the bytecode
 * @param capacity Capacity of the buffer in bytes
 * @param read_len Optional output for the number of bytes read (may be NULL)
 * @return openblink_status_t OPENBLINK_STATUS_OK on success,
 * OPENBLINK_STATUS_NOT_FOUND if the slot is empty
 */
openblink_status_t openblink_slot_load(openblink_slot_t slot, void* data,
                                       size_t capacity,
                                       openblink_length_t* read_len);

/**
 * @brief Gets the length of the bytecode stored in the specified slot
 *
 * @param slot Slot to query (1..OPENBLINK_SLOT_COUNT)
 * @param len Output for the stored length in bytes
 * @return openblink_status_t OPENBLINK_STATUS_OK on success,
 * OPENBLINK_STATUS_NOT_FOUND if the slot is empty
 */
openblink_status_t openblink_slot_get_data_length(openblink_slot_t slot,
                                                  openblink_length_t* len);

/**
 * @brief Deletes the bytecode stored in the specified slot
 *
 * @param slot Slot to delete (1..OPENBLINK_SLOT_COUNT)
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_slot_delete(openblink_slot_t slot);

/* ---- HAL (implemented by the platform) ---------------------------------- */

/**
 * @brief Initializes the non-volatile storage backend
 *
 * @details Must be called by the platform before any other storage HAL
 * function is used. The storage HAL functions must be thread-safe.
 *
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_hal_storage_init(void);

/**
 * @brief Reads bytecode for a slot from non-volatile storage
 *
 * @param slot Source slot (1..OPENBLINK_SLOT_COUNT)
 * @param data Buffer for the bytecode
 * @param capacity Capacity of the buffer in bytes
 * @param read_len Output for the number of bytes read
 * @return openblink_status_t OPENBLINK_STATUS_OK on success,
 * OPENBLINK_STATUS_NOT_FOUND if the slot is empty
 */
openblink_status_t openblink_hal_storage_read(openblink_slot_t slot, void* data,
                                              size_t capacity,
                                              openblink_length_t* read_len);

/**
 * @brief Writes bytecode for a slot to non-volatile storage
 *
 * @param slot Target slot (1..OPENBLINK_SLOT_COUNT)
 * @param data Pointer to the bytecode
 * @param len Length of the bytecode in bytes
 * @param written_len Output for the number of bytes written
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_hal_storage_write(openblink_slot_t slot,
                                               const void* data,
                                               openblink_length_t len,
                                               openblink_length_t* written_len);

/**
 * @brief Gets the stored bytecode length for a slot
 *
 * @param slot Slot to query (1..OPENBLINK_SLOT_COUNT)
 * @param len Output for the stored length in bytes
 * @return openblink_status_t OPENBLINK_STATUS_OK on success,
 * OPENBLINK_STATUS_NOT_FOUND if the slot is empty
 */
openblink_status_t openblink_hal_storage_get_length(openblink_slot_t slot,
                                                    openblink_length_t* len);

/**
 * @brief Deletes the stored bytecode for a slot
 *
 * @param slot Slot to delete (1..OPENBLINK_SLOT_COUNT)
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_hal_storage_delete(openblink_slot_t slot);

/**
 * @brief Sends a Blink protocol response to the connected host
 *
 * @details Called by the core to deliver "OK slot:<n>" and error responses.
 * When no host is connected or subscribed, the platform may drop the data and
 * return OPENBLINK_STATUS_OK.
 *
 * @param data Pointer to the response payload
 * @param len Length of the response payload in bytes
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_hal_send_response(const void* data,
                                               openblink_length_t len);

/**
 * @brief Reboots the device
 *
 * @details Invoked when an 'R' command is received. This function is not
 * expected to return on success.
 *
 * @return openblink_status_t OPENBLINK_STATUS_ERROR if the reboot failed
 */
openblink_status_t openblink_hal_reboot(void);

#ifdef __cplusplus
}
#endif

#endif /* OPENBLINK_BLINK_H */

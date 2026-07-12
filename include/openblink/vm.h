/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file vm.h
 * @brief mruby/c VM lifecycle management
 * @details Declares the VM main loop, restart control, the VM lock used by
 * Blink.lock/Blink.unlock, and the HAL functions that a platform must
 * implement to support them.
 */
#ifndef OPENBLINK_VM_H
#define OPENBLINK_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Core API ----------------------------------------------------------- */

/**
 * @brief Runs the mruby/c VM forever
 *
 * @details Initializes the VM, loads the bytecode of every slot, creates the
 * tasks and executes them. When all tasks finish (e.g. after
 * openblink_vm_restart()), the VM is cleaned up and started again with the
 * latest stored bytecode. This function never returns; the platform must call
 * it from a dedicated thread or its main loop.
 */
void openblink_vm_main(void);

/**
 * @brief Requests a restart of the mruby/c VM
 *
 * @details Terminates all running tasks so that openblink_vm_main() reloads
 * the stored bytecode. Fails when the VM lock is held (e.g. by Blink.lock)
 * and cannot be acquired within OPENBLINK_VM_RESTART_TIMEOUT_MS.
 *
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_vm_restart(void);

/**
 * @brief Acquires the VM lock (Blink.lock)
 *
 * @details While the lock is held, openblink_vm_restart() fails, protecting
 * critical sections of Ruby code from being interrupted by a reload.
 *
 * @return bool true if the lock was acquired
 */
bool openblink_vm_lock(void);

/**
 * @brief Releases the VM lock (Blink.unlock)
 *
 * @return bool true if the lock was released
 */
bool openblink_vm_unlock(void);

/* ---- HAL (implemented by the platform) ---------------------------------- */

/**
 * @brief Acquires the platform mutex protecting VM restarts
 *
 * @details The mutex must support acquisition from the VM task context and
 * from the context that calls openblink_vm_restart().
 *
 * @param timeout_ms Maximum time to wait for the lock in milliseconds
 * @return bool true if the lock was acquired
 */
bool openblink_hal_vm_lock(uint32_t timeout_ms);

/**
 * @brief Releases the platform mutex protecting VM restarts
 *
 * @return bool true if the lock was released
 */
bool openblink_hal_vm_unlock(void);

/**
 * @brief Defines platform-specific mruby/c classes and methods
 *
 * @details Called once per VM start, after mrbc_init() and before tasks are
 * created. The platform registers its hardware APIs (e.g. LED, Input) here.
 *
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_hal_define_api(void);

/**
 * @brief Provides the factory default bytecode for a slot
 *
 * @details Called when non-volatile storage has no bytecode for the slot.
 * The platform may return OPENBLINK_STATUS_NOT_FOUND if no default program
 * exists for the slot.
 *
 * @param slot Slot to load (1..OPENBLINK_SLOT_COUNT)
 * @param data Buffer for the bytecode
 * @param capacity Capacity of the buffer in bytes
 * @param len Output for the number of bytes provided
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_hal_load_default_bytecode(openblink_slot_t slot,
                                                       uint8_t* data,
                                                       size_t capacity,
                                                       openblink_length_t* len);

#ifdef __cplusplus
}
#endif

#endif /* OPENBLINK_VM_H */

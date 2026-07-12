/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file config.h
 * @brief Compile-time configuration for the OpenBlink core
 * @details Every constant can be overridden by defining it before this header
 * is included (e.g. via compiler flags). All values must be integer constant
 * expressions; they are validated below.
 */
#ifndef OPENBLINK_CONFIG_H
#define OPENBLINK_CONFIG_H

/**
 * @brief Maximum size of the bytecode accepted for one slot in bytes
 */
#ifndef OPENBLINK_MAX_BYTECODE_SIZE
#define OPENBLINK_MAX_BYTECODE_SIZE 4016
#endif

/**
 * @brief Number of bytecode slots (slots are numbered 1..OPENBLINK_SLOT_COUNT)
 */
#ifndef OPENBLINK_SLOT_COUNT
#define OPENBLINK_SLOT_COUNT 2
#endif

/**
 * @brief Size of the mruby/c VM heap in bytes
 */
#ifndef OPENBLINK_VM_HEAP_SIZE
#define OPENBLINK_VM_HEAP_SIZE (15 * 1024)
#endif

/**
 * @brief Timeout for Blink.lock acquisition in milliseconds
 */
#ifndef OPENBLINK_VM_LOCK_TIMEOUT_MS
#define OPENBLINK_VM_LOCK_TIMEOUT_MS 1
#endif

/**
 * @brief Timeout for acquiring the VM lock during a restart in milliseconds
 */
#ifndef OPENBLINK_VM_RESTART_TIMEOUT_MS
#define OPENBLINK_VM_RESTART_TIMEOUT_MS 1000
#endif

/* ---- Validation -------------------------------------------------------- */

#if OPENBLINK_MAX_BYTECODE_SIZE <= 0
#error "OPENBLINK_MAX_BYTECODE_SIZE must be positive"
#endif

/* The wire protocol carries offset and size as 16-bit values and a 'D' frame
 * adds a 6-byte header, so the buffer must stay addressable within 16 bits. */
#if OPENBLINK_MAX_BYTECODE_SIZE > 65529
#error "OPENBLINK_MAX_BYTECODE_SIZE must not exceed 65529"
#endif

/* The wire protocol carries the slot number as a single byte (1-origin). */
#if OPENBLINK_SLOT_COUNT < 1 || OPENBLINK_SLOT_COUNT > 255
#error "OPENBLINK_SLOT_COUNT must be between 1 and 255"
#endif

#if OPENBLINK_VM_HEAP_SIZE <= 0
#error "OPENBLINK_VM_HEAP_SIZE must be positive"
#endif

#if OPENBLINK_VM_LOCK_TIMEOUT_MS < 0
#error "OPENBLINK_VM_LOCK_TIMEOUT_MS must not be negative"
#endif

#if OPENBLINK_VM_RESTART_TIMEOUT_MS < 0
#error "OPENBLINK_VM_RESTART_TIMEOUT_MS must not be negative"
#endif

#endif /* OPENBLINK_CONFIG_H */

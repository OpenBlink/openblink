/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file types.h
 * @brief Common fixed-width types and status codes for the OpenBlink core
 * @details Defines the platform-independent types shared by every OpenBlink
 * core API and HAL function. Only standard C headers are used.
 */
#ifndef OPENBLINK_TYPES_H
#define OPENBLINK_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status code returned by OpenBlink core API and HAL functions
 */
typedef enum {
  OPENBLINK_STATUS_OK = 0,          /**< Operation succeeded */
  OPENBLINK_STATUS_ERROR = 1,       /**< Unspecified failure */
  OPENBLINK_STATUS_INVALID_ARG = 2, /**< Invalid argument (e.g. bad slot) */
  OPENBLINK_STATUS_NOT_FOUND = 3,   /**< Requested data does not exist */
} openblink_status_t;

/**
 * @brief Bytecode slot number (1-origin)
 */
typedef uint8_t openblink_slot_t;

/**
 * @brief Length of bytecode or protocol payloads in bytes
 */
typedef uint16_t openblink_length_t;

#ifdef __cplusplus
}
#endif

#endif /* OPENBLINK_TYPES_H */

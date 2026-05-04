/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file comm.h
 * @brief Communication management interface
 * @details Defines functions for managing BLE communication
 */
#ifndef APP_COMM_H
#define APP_COMM_H

#include "../lib/fn.h"

/**
 * @brief Initializes the communication subsystem
 *
 * @return fn_t kSuccess if successful, kFailure otherwise
 */
fn_t comm_init(void);

/**
 * @brief Disconnects the current BLE connection
 *
 * @return fn_t kSuccess if successful, kFailure otherwise
 */
fn_t comm_disconnect(void);

#endif  // APP_COMM_H

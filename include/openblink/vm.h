/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file mrubyc_vm.h
 * @brief mruby/c virtual machine management
 * @details Provides functions for controlling the mruby/c virtual machine
 */
#ifndef APP_MRUBYC_VM_H
#define APP_MRUBYC_VM_H

#include <stdbool.h>

#include "../lib/fn.h"

/**
 * @brief Restart the mruby/c virtual machine
 *
 * @details The VM will restart
 *
 * @return fn_t kSuccess if successful
 */
fn_t app_mrubyc_vm_restart(void);

#endif

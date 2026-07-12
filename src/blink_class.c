/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file blink.c
 * @brief Implementation of Blink API for mruby/c
 * @details Implements the Blink class and methods for mruby/c scripts
 */
#include "blink.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "../../mrubyc/src/mrubyc.h"
#include "../lib/fn.h"

LOG_MODULE_REGISTER(api_blink, LOG_LEVEL_WRN);

/** @brief External mutex for mruby/c VM restart operations */
extern struct k_mutex mutex_mrubyc_vm_restart;

/**
 * @brief Forward declaration for reload status getter method
 *
 * @param vm The mruby/c VM instance
 * @param v The value array
 * @param argc The argument count
 */
static void c_get_reload(mrb_vm* vm, mrb_value* v, int argc);
static void c_lock_blink(mrb_vm* vm, mrb_value* v, int argc);
static void c_unlock_blink(mrb_vm* vm, mrb_value* v, int argc);

/**
 * @brief Defines the Blink class and methods for mruby/c
 *
 * @return fn_t kSuccess if successful, kFailure otherwise
 */
fn_t api_blink_define(void) {
  mrb_class* class_blink;
  class_blink = mrbc_define_class(0, "Blink", mrbc_class_object);
  mrbc_define_method(0, class_blink, "req_reload?", c_get_reload);
  mrbc_define_method(0, class_blink, "lock", c_lock_blink);
  mrbc_define_method(0, class_blink, "unlock", c_unlock_blink);
  return kSuccess;
}

/**
 * @brief Gets the reload status from the mruby/c VM
 *
 * @param vm The mruby/c VM instance
 * @param v The value array
 * @param argc The argument count
 */
static void c_get_reload(mrb_vm* vm, mrb_value* v, int argc) {
  LOG_DBG("Call deprecated method: Blink.req_reload?");
  SET_FALSE_RETURN();
}

/**
 * @brief Implementation of the lock method for the Blink class
 *
 * Lock the blink
 *
 * @param vm Pointer to the mruby/c VM
 * @param v Pointer to the method arguments
 * @param argc Number of arguments
 */
static void c_lock_blink(mrb_vm* vm, mrb_value* v, int argc) {
  if (0 == k_mutex_lock(&mutex_mrubyc_vm_restart, K_MSEC(1))) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

/**
 * @brief Implementation of the unlock method for the Blink class
 *
 * Unlock the blink
 *
 * @param vm Pointer to the mruby/c VM
 * @param v Pointer to the method arguments
 * @param argc Number of arguments
 */
static void c_unlock_blink(mrb_vm* vm, mrb_value* v, int argc) {
  if (0 == k_mutex_unlock(&mutex_mrubyc_vm_restart)) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

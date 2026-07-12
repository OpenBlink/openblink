/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file blink_class.c
 * @brief Blink class for mruby/c
 * @details Implements the Blink class exposing Blink.lock and Blink.unlock to
 * Ruby scripts so critical sections can be protected from VM reloads.
 */
#include <mrubyc.h>

#include "internal.h"
#include "openblink/vm.h"

/**
 * @brief Implementation of the lock method for the Blink class
 *
 * @param vm Pointer to the mruby/c VM
 * @param v Pointer to the method arguments
 * @param argc Number of arguments
 */
static void c_lock_blink(mrb_vm* vm, mrb_value* v, int argc) {
  (void)vm;
  (void)argc;
  if (openblink_vm_lock()) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

/**
 * @brief Implementation of the unlock method for the Blink class
 *
 * @param vm Pointer to the mruby/c VM
 * @param v Pointer to the method arguments
 * @param argc Number of arguments
 */
static void c_unlock_blink(mrb_vm* vm, mrb_value* v, int argc) {
  (void)vm;
  (void)argc;
  if (openblink_vm_unlock()) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

openblink_status_t openblink_define_blink_class(void) {
  mrb_class* class_blink = mrbc_define_class(0, "Blink", mrbc_class_object);
  if (NULL == class_blink) {
    return OPENBLINK_STATUS_ERROR;
  }
  mrbc_define_method(0, class_blink, "lock", c_lock_blink);
  mrbc_define_method(0, class_blink, "unlock", c_unlock_blink);
  return OPENBLINK_STATUS_OK;
}

/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright (c) 2025 ViXion Inc. All Rights Reserved.
 */
/**
 * @file internal.h
 * @brief Internal declarations shared between OpenBlink core sources
 */
#ifndef OPENBLINK_INTERNAL_H
#define OPENBLINK_INTERNAL_H

#include "openblink/types.h"

/**
 * @brief Defines the Blink class (Blink.lock / Blink.unlock) for mruby/c
 *
 * @return openblink_status_t OPENBLINK_STATUS_OK on success
 */
openblink_status_t openblink_define_blink_class(void);

#endif /* OPENBLINK_INTERNAL_H */

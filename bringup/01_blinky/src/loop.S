/*
 * Copyright 2022, Daniel DeGrasse
 *
 * SPDX-License-Identifier: Apache-2.0
 */

.global z_arm_platform_init

.thumb
z_arm_platform_init:
    mov r0, #0
_debug_spin:
    cmp r0, #0
    beq _debug_spin
    bx lr
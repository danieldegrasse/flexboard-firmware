# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Add custom board root dir
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(is31fl3733_matrix)

target_sources(app PRIVATE src/main.c)

FILE(GLOB matrix_sources src/led_matrix/*.c)
target_sources(app PRIVATE ${matrix_sources})

target_include_directories(app PRIVATE src/led_matrix)

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(OpenBlink)

# Generate mrubyc autogen files
add_custom_target(mrubyc_autogen
    COMMAND make autogen
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mrubyc
    COMMENT "Generating mrubyc autogen files"
)

add_dependencies(app mrubyc_autogen)

target_sources(app PRIVATE
                    src/main.c
                    src/app/blink.c
                    src/app/comm.c
                    src/app/init.c
                    src/app/mrubyc_vm.c
                    src/app/storage.c
                    src/api/api.c
                    src/api/ble.c
                    src/api/blink.c
                    src/api/input.c
                    src/api/led.c
                    src/api/symbol.c
                    src/drv/ble.c
                    src/drv/ble_blink.c
                    src/drv/gpio.c
                    src/lib/mrubyc/hal.c
                    mrubyc/src/alloc.c
                    mrubyc/src/c_array.c
                    mrubyc/src/c_hash.c
                    mrubyc/src/c_math.c
                    mrubyc/src/c_numeric.c
                    mrubyc/src/c_object.c
                    mrubyc/src/c_proc.c
                    mrubyc/src/c_range.c
                    mrubyc/src/c_string.c
                    mrubyc/src/class.c
                    mrubyc/src/console.c
                    mrubyc/src/error.c
                    mrubyc/src/global.c
                    mrubyc/src/keyvalue.c
                    mrubyc/src/load.c
                    mrubyc/src/mrblib.c
                    mrubyc/src/rrt0.c
                    mrubyc/src/symbol.c
                    mrubyc/src/value.c
                    mrubyc/src/vm.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/mrubyc ${CMAKE_CURRENT_SOURCE_DIR}/mrubyc/src)

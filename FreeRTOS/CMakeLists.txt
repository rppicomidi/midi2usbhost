cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(BOARD pico_sdk)
set(TINYUSB_OPT_OS OPT_OS_FREERTOS)
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
if (NOT FREERTOS_KERNEL_PATH AND NOT DEFINED ENV{FREERTOS_KERNEL_PATH})
    message("Error: must define FREERTOS_KERNEL_PATH. Quitting")
    return()
endif()

# locate the copy of the FreeRTOS Kernel
include(FreeRTOS_Kernel_import.cmake)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

project(midi2usbhost C CXX ASM)


add_executable(midi2usbhost
    midi2usbhost.c
    )

target_include_directories(midi2usbhost PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_options(midi2usbhost PRIVATE -Xlinker --print-memory-usage)
target_compile_options(midi2usbhost PRIVATE -Wall -Wextra)
target_link_libraries(midi2usbhost tinyusb_host FreeRTOS-Kernel-Heap4 tinyusb_board pico_stdlib)
target_compile_definitions(midi2usbhost PRIVATE
    configSUPPORT_STATIC_ALLOCATION=0
    configSUPPORT_DYNAMIC_ALLOCATION=1
    MIDI_UART_NUM=1
    )
pico_add_extra_outputs(midi2usbhost)


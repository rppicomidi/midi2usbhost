cmake_minimum_required(VERSION 3.13)

set(BOARD pico_sdk)
include(pico_sdk_import.cmake)

set(target_proj midi2usbhost)
project(${target_proj} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/midi_uart_lib)

add_executable(${target_proj}
    midi2usbhost.c
)

pico_enable_stdio_uart(${target_proj} 1)

target_include_directories(${target_proj} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_options(${target_proj} PRIVATE -Xlinker --print-memory-usage)
target_compile_options(${target_proj} PRIVATE -Wall -Wextra -DCFG_TUSB_DEBUG=1)
target_link_libraries(${target_proj} midi_uart_lib tinyusb_host tinyusb_board ring_buffer_lib pico_stdlib)

pico_add_extra_outputs(${target_proj})


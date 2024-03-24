/**
 * @file ring_buffer_lib_config.h
 * @brief Put your custom configuration #defines for the ring buffer library here
 *
 * In this example, the ring buffers are only used for UART1, so a buffer can only
 * be changed in the UART1 IRQ handler context and the main non-IRQ context. Therefore,
 * critical sections only have to manage the the UART1 IRQ, not all IRQs.
 * 
 * MIT License
 *
 * Copyright (c) 2022 rppicomidi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */ 
 
#ifndef RING_BUFFER_LIB_CONFIG
#define RING_BUFFER_LIB_CONFIG

// You can define a larger ring buffer size type if you want
// RING_BUFFER_SIZE_TYPE has to be able to hold a value less
// than or equal to the maximum number of bytes in a ring buffer
// Uncomment and modify the next line if the default (uint8_t)
// is not big enough
// #define RING_BUFFER_SIZE_TYPE uint8_t

// If you don't need to support multiple cores modifying the
// ring buffer, set RING_BUFFER_MULTICORE_SUPPORT to 0; it
// will save time and space
// Otherwise set it to 1. Uncomment the next line to enable
// multicore support
// #define RING_BUFFER_MULTICORE_SUPPORT 1

// If RING_BUFFER_MULTICORE_SUPPORT is 0, then safe version
// of ring buffer calls by default disable all IRQs on the
// current core, manipulate the buffer, and then restore IRQs.
// That is probably overkill for most applications. For
// example, if you are using this ring buffer with a peripheral
// driver, you can store the IRQ number to disable in the
// critical_section_data field of the ring_buffer structure
// in the ring_buffer_init() function call and then use
// that value to enable or disable only that peripheral IRQ.
// Uncomment the example below to do that
#include "hardware/irq.h"

#define RING_BUFFER_ENTER_CRITICAL(X) \
    do {irq_set_enabled(ring_buf->critical_section_data, false);} while (0)

#define RING_BUFFER_EXIT_CRITICAL(X) \
    do {irq_set_enabled(ring_buf->critical_section_data, true);} while (0)

#endif

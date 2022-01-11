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

#include "hardware/irq.h"

#ifdef RING_BUFFER_ENTER_CRITICAL
#undef RING_BUFFER_ENTER_CRITICAL
#endif

#define RING_BUFFER_ENTER_CRITICAL(X) \
    do {irq_set_enabled(ring_buf->critical_section_data, false);} while (0)

#ifdef RING_BUFFER_EXIT_CRITICAL
#undef RING_BUFFER_EXIT_CRITICAL
#endif

#define RING_BUFFER_EXIT_CRITICAL(X) \
    do {irq_set_enabled(ring_buf->critical_section_data, true);} while (0)


#endif
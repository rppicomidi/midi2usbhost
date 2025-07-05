/**
 * @file midi_uart_lib_config.h
 * @brief this file choses how many UARTs the midi_uart_lib library supports
 * 
 * MIT License

 * Copyright (c) 2022 rppicomidi

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */


#ifndef MIDI_UART_LIB_CONFIG_H
#define MIDI_UART_LIB_CONFIG_H

// The standard MIDI UART baud rate is 31250. If you want to do
// something special with the MIDI UART LIB, you can change this
#define MIDI_UART_LIB_BAUD_RATE 31250

// The standard MIDI_UART_RING_BUFFER_LENGTH is defined in the driver code
// If you want to customize the buffers, uncomment the following line and
// set to a value you like
// #define MIDI_UART_RING_BUFFER_LENGTH 128
#endif

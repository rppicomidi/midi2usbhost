/**
 * @file Pico-USB-Host-MIDI-Adapter.c
 * @brief A USB Host to Serial Port MIDI adapter that runs on a Raspberry Pi
 * Pico board
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
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "midi_uart_lib.h"
#include "bsp/board_api.h"
#include "tusb.h"

// UART selection Pin mapping. You can move these for your design if you want to
// Make sure all these values are consistent with your choice of midi_uart
// The default is to use UART 1, but you are free to use UART 0 if you make
// the changes in the CMakeLists.txt file or in your environment. Note
// that if you use UART0, then serial port debug will not be enabled
#ifndef MIDI_UART_NUM
#define MIDI_UART_NUM 1
#endif
#ifndef MIDI_UART_TX_GPIO
#define MIDI_UART_TX_GPIO 4
#endif
#ifndef MIDI_UART_RX_GPIO
#define MIDI_UART_RX_GPIO 5
#endif

static void *midi_uart_instance;
static uint8_t dev_idx = TUSB_INDEX_INVALID_8;
static bool update_strings = false;

static void blink_led(void)
{
    static absolute_time_t previous_timestamp = {0};

    static bool led_state = false;

    absolute_time_t now = get_absolute_time();
    
    int64_t diff = absolute_time_diff_us(previous_timestamp, now);
    if (diff > 1000000) {
        board_led_write(led_state);
        led_state = !led_state;
        previous_timestamp = now;
    }
}

static void poll_midi_uart_rx(bool connected)
{
    uint8_t rx[48];
    // Pull any bytes received on the MIDI UART out of the receive buffer and
    // send them out via USB MIDI on virtual cable 0
    uint8_t nread = midi_uart_poll_rx_buffer(midi_uart_instance, rx, sizeof(rx));
    if (nread > 0 && connected && tuh_midi_get_rx_cable_count(dev_idx) >= 1)
    {
        uint32_t nwritten = tuh_midi_stream_write(dev_idx, 0,rx, nread);
        if (nwritten != nread) {
            TU_LOG1("Warning: Dropped %lu bytes receiving from UART MIDI In\r\n", nread - nwritten);
        }
    }
}

// This routine converts only a limited set of UTF-16LE to UTF-8
// It is good enough for US English USB string descriptors
// If you need something better, please file an issue in this project.
static void print_string_descriptor(uint16_t *buffer)
{
    uint8_t len = ((*buffer) & 0xFF)/2;
    for (uint8_t idx = 1; idx < len; idx++) {
        printf("%c", buffer[idx]);
    }
    printf("\r\n");
}

int main() {

    bi_decl(bi_program_description("Provide a USB host interface for Serial Port MIDI."));
    bi_decl(bi_2pins_with_names(MIDI_UART_TX_GPIO, "MIDI UART TX", MIDI_UART_RX_GPIO, "MIDI UART RX"));

    board_init();
    printf("Pico MIDI Host to MIDI UART Adapter\r\n");
    tusb_init();

    // Map the pins to functions
    midi_uart_instance = midi_uart_configure(MIDI_UART_NUM, MIDI_UART_TX_GPIO, MIDI_UART_RX_GPIO);
    printf("Configured MIDI UART %u for 31250 baud\r\n", MIDI_UART_NUM);
    while (1) {
        tuh_task();

        blink_led();
        bool connected = dev_idx != TUSB_INDEX_INVALID_8 && tuh_midi_mounted(dev_idx);

        poll_midi_uart_rx(connected);
        if (connected)
            tuh_midi_write_flush(dev_idx);
        midi_uart_drain_tx_buffer(midi_uart_instance);
        if (connected && update_strings) {
            uint16_t buffer[128];
            update_strings = false;
            tuh_itf_info_t info;
            if (!tuh_midi_itf_get_info(dev_idx, &info))
                panic("tuh_midi_itf_get_info failed\r\n");
            if (tuh_descriptor_get_string_langid_sync(info.daddr, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
                uint16_t langid = buffer[1];
                if (tuh_descriptor_get_manufacturer_string_sync(info.daddr, langid, buffer, sizeof(buffer))== XFER_RESULT_SUCCESS) {
                    printf("manufacturer: ");
                    print_string_descriptor(buffer);
                }
                if (tuh_descriptor_get_product_string_sync(info.daddr, langid, buffer, sizeof(buffer))== XFER_RESULT_SUCCESS) {
                    printf("product: ");
                    print_string_descriptor(buffer);
                }
                if (tuh_descriptor_get_serial_string_sync(info.daddr, langid, buffer, sizeof(buffer))== XFER_RESULT_SUCCESS) {
                    printf("serial: ");
                    print_string_descriptor(buffer);
                }
            }
            
        }
    }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+
// Invoked when device with MIDI interface is mounted
void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t* mount_cb_data)
{
  printf("MIDI Device Index = %u, MIDI device address = %u, %u IN cables, OUT %u cables\r\n", idx,
      mount_cb_data->daddr, mount_cb_data->rx_cable_count, mount_cb_data->tx_cable_count);

  if (dev_idx == TUSB_INDEX_INVALID_8) {
    // then no MIDI device is currently connected
    dev_idx = idx;
  }
  else {
    printf("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled\r\n");
  }
  update_strings = true;
}

// Invoked when device with MIDI interface is un-mounted
void tuh_midi_umount_cb(uint8_t idx)
{
  if (idx == dev_idx) {
    dev_idx = TUSB_INDEX_INVALID_8;
    printf("MIDI Device Index = %u is unmounted\r\n", idx);
  }
  else {
    printf("Unused MIDI Device Index  %u is unmounted\r\n", idx);
  }
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t num_bytes)
{
    if (dev_idx == idx)
    {
        if (num_bytes != 0)
        {
            uint8_t cable_num;
            uint8_t buffer[48];
            while (1) {
                uint32_t bytes_read = tuh_midi_stream_read(idx, &cable_num, buffer, sizeof(buffer));
                if (bytes_read == 0)
                    return;
                if (cable_num == 0) {
                    uint8_t npushed = midi_uart_write_tx_buffer(midi_uart_instance,buffer,bytes_read);
                    if (npushed != bytes_read) {
                        TU_LOG1("Warning: Dropped %lu bytes sending to UART MIDI Out\r\n", bytes_read - npushed);
                    }
                }
            }
        }
    }
}

void tuh_midi_tx_cb(uint8_t idx, uint32_t num_bytes)
{
    (void)idx;
    (void)num_bytes;
}
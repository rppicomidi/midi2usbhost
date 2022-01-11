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
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/binary_info.h"
#include "pico/sync.h"
#include "ring_buffer_lib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "class/midi/midi_host.h"
// On-board LED mapping. If no LED, set to NO_LED_GPIO
const uint NO_LED_GPIO = 255;
const uint LED_GPIO = 25;
// UART selection Pin mapping. You can move these for your design if you want to
// Make sure all these values are consistent with your choice of midi_uart
#define midi_uart (uart1)
#define UART_IRQ UART1_IRQ
const uint MIDI_UART_TX_GPIO = 4;
const uint MIDI_UART_RX_GPIO = 5;

#define MIDI_UART_RING_BUFFER_LENGTH 128
static ring_buffer_t midi_uart_rx, midi_uart_tx;
static uint8_t midi_uart_rx_buf[MIDI_UART_RING_BUFFER_LENGTH], midi_uart_tx_buf[MIDI_UART_RING_BUFFER_LENGTH];
static uint midi_uart_rx_lock, midi_uart_tx_lock;

static uint8_t midi_dev_addr = 0;

static void on_midi_uart_irq()
{
    uint32_t status = uart_get_hw(midi_uart)->mis; // the reason this irq happened
    if (status & UART_UARTMIS_RXMIS_BITS) {
        // Got a byte
        uint32_t dr = uart_get_hw(midi_uart)->dr;
        uint8_t val = dr & 0xff;
        if ((dr & !0xf00) == 0) {
            // then data read was OK.
            // ignore the return value. If the buffer is full, nothing can be done
            (void)ring_buffer_push_unsafe(&midi_uart_rx, &val, 1);
        }
    }
    if (status & UART_UARTMIS_TXMIS_BITS) {
        // ready to transmit
        uint8_t val;
        RING_BUFFER_SIZE_TYPE npopped = ring_buffer_pop_unsafe(&midi_uart_tx, &val, 1);
        if (npopped == 0) {
            // No data to send; disable the interrupt
            uart_set_irq_enables(midi_uart, true, false);
        }
        else {
            // send the byte
            uart_get_hw(midi_uart)->dr = val;
        }
    }
}

static uint32_t configure_midi_uart()
{
    // Set up the UART hardware
    uint32_t midi_baud = uart_init(midi_uart, 31250);
    uart_set_format(midi_uart, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(midi_uart, false, false);
    uart_set_fifo_enabled(midi_uart, false);
    uart_set_translate_crlf(midi_uart, false);

    gpio_set_function(MIDI_UART_TX_GPIO, GPIO_FUNC_UART);
    gpio_set_function(MIDI_UART_RX_GPIO, GPIO_FUNC_UART);

    // Prepare the MIDI UART ring buffers and interrupt handler and enable interrupts
    ring_buffer_init(&midi_uart_rx, midi_uart_rx_buf, MIDI_UART_RING_BUFFER_LENGTH, UART_IRQ);
    ring_buffer_init(&midi_uart_tx, midi_uart_tx_buf, MIDI_UART_RING_BUFFER_LENGTH, UART_IRQ);

    irq_set_exclusive_handler(UART_IRQ, on_midi_uart_irq);
    irq_set_enabled(UART_IRQ, true);

    // enable the rx IRQ. No data to send yet.
    uart_set_irq_enables(midi_uart, true, false);
    return midi_baud;
}

static void blink_led(void)
{
    static absolute_time_t previous_timestamp = {0};

    static bool led_state = false;

    // This design has no on-board LED
    if (NO_LED_GPIO == LED_GPIO)
        return;
    absolute_time_t now = get_absolute_time();
    
    int64_t diff = absolute_time_diff_us(previous_timestamp, now);
    if (diff > 1000000) {
        gpio_put(LED_GPIO, led_state);
        led_state = !led_state;
        previous_timestamp = now;
    }
}


static void drain_midi_uart_buffer(void)
{
    // disable UART interrupts because checking if still transmitting from the buffer
    irq_set_enabled(UART_IRQ, false);
    // can use the unsafe version of ring_buffer_is_empty_unsafe() because UART IRQ
    // is disabled
    if (!ring_buffer_is_empty_unsafe(&midi_uart_tx)) {
        uint8_t val;
        if ((uart_get_hw(midi_uart)->imsc & UART_UARTIMSC_TXIM_BITS) == 0) {
            // then last transmission is complete. Kick start a new one
            RING_BUFFER_SIZE_TYPE result = ring_buffer_pop_unsafe(&midi_uart_tx, &val, 1);
            assert(result == 1);
            uart_get_hw(midi_uart)->dr = val;
            uart_set_irq_enables(midi_uart, true, true);
        }
    }
    irq_set_enabled(UART_IRQ, true);
}

static void poll_usb_rx(void)
{
  // device must be attached and have at least one endpoint ready to receive a message
  if (!midi_dev_addr || !tuh_midi_configured(midi_dev_addr))
  {
    return;
  }
  if (tuh_midih_get_num_rx_cables(midi_dev_addr) < 1)
  {
    return;
  }
  tuh_midi_read_poll(midi_dev_addr);
}

int main() {

    bi_decl(bi_program_description("Provide a USB host interface for Serial Port MIDI."));
    bi_decl(bi_1pin_with_name(LED_GPIO, "On-board LED"));
    bi_decl(bi_2pins_with_names(MIDI_UART_TX_GPIO, "MIDI UART TX", MIDI_UART_RX_GPIO, "MIDI UART RX"));

    //stdio_init_all();
    board_init();
    printf("Pico MIDI Host to MIDI UART Adapter\r\n");
    tusb_init();

    // Map the pins to functions
    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    uint32_t midi_baud = configure_midi_uart();
    printf("Configured MIDI UART %u for %u baud\r\n", uart_get_index(midi_uart), midi_baud);
    while (1) {
        tuh_task();

        blink_led();

        uint8_t rx[48];
        uint8_t nread = ring_buffer_pop(&midi_uart_rx, rx, sizeof(rx));
        bool connected = midi_dev_addr != 0 && tuh_midi_configured(midi_dev_addr);
        if (nread > 0 && connected && tuh_midih_get_num_tx_cables(midi_dev_addr) >= 1)
        {
            // Send it out
            uint32_t nwritten = tuh_midi_stream_write(midi_dev_addr, 0,rx, nread);
            if (nwritten != nread) {
                TU_LOG1("Warning: Dropped %d bytes receiving from UART MIDI In\r\n", nread - nwritten);
            }
        }
        if (connected)
            tuh_midi_stream_flush(midi_dev_addr);
        poll_usb_rx();
        drain_midi_uart_buffer();
    }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_midi_mount_cb(uint8_t dev_addr, uint8_t in_ep, uint8_t out_ep, uint8_t num_cables_rx, uint16_t num_cables_tx)
{
  printf("MIDI device address = %u, IN endpoint %u has %u cables, OUT endpoint %u has %u cables\r\n",
      dev_addr, in_ep & 0xf, num_cables_rx, out_ep & 0xf, num_cables_tx);

  midi_dev_addr = dev_addr;
}

// Invoked when device with hid interface is un-mounted
void tuh_midi_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  midi_dev_addr = 0;
  printf("MIDI device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

void tuh_midi_rx_cb(uint8_t dev_addr, uint32_t num_packets)
{
    if (midi_dev_addr == dev_addr)
    {
        if (num_packets != 0)
        {
            uint8_t cable_num;
            uint8_t buffer[48];
            uint32_t bytes_read = tuh_midi_stream_read(dev_addr, &cable_num, buffer, sizeof(buffer));
            RING_BUFFER_SIZE_TYPE npushed = ring_buffer_push(&midi_uart_tx,buffer,bytes_read);
            if (npushed != bytes_read) {
                TU_LOG1("Warning: Dropped %d bytes sending to UART MIDI Out\r\n", bytes_read - npushed);
            }
        }
    }
}

void tuh_midi_tx_cb(uint8_t dev_addr)
{

}
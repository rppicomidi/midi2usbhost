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
#include "pico/multicore.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pico/binary_info.h"
#include "bsp/board_api.h"
#include "tusb.h"

#define MIDI_UART_QUEUE_LENGTH 128
typedef struct MIDI_UART_S {
    // UART selection and pin mapping.
    uart_inst_t *midi_uart;
    uint midi_uart_irq;
    uint midi_uart_tx_gpio;
    uint midi_uart_rx_gpio;
    uint tx_overflow_gpio;
    uint rx_overflow_gpio;
    QueueHandle_t midi_uart_tx_queue;
    QueueHandle_t midi_uart_rx_queue;
} MIDI_UART_T;
static MIDI_UART_T midi_uart0, midi_uart1, *the_midi_uart;
static void on_midi_uart_irq(MIDI_UART_T *midi_uart);

// On-board LED mapping. If no LED, set to NO_LED_GPIO
const uint LED_RX_OVERFLOW_GPIO = 2;
const uint LED_TX_OVERFLOW_GPIO = 3;

// UART selection Pin mapping. You can move these for your design if you want to
// The default is to use UART 1, but you are free to use UART 0 if you make
// the changes in the CMakeLists.txt file or in your environment. Note
// that if you use UART0, then serial port debug will not be enabled
// UART selection Pin mapping. You can move these for your design if you want to
// Make sure all these values are consistent with your choice of midi_uart
#ifndef MIDI_UART_NUM
#define MIDI_UART_NUM 1
#elif MIDI_UART_NUM > 1 || MIDI_UART_NUM < 0
#error "MIDI_UART_NUM must be 0 or 1"
#endif
#if MIDI_UART_NUM==0
const uint MIDI_UART_TX_GPIO = 0;
const uint MIDI_UART_RX_GPIO = 1;
#else
const uint MIDI_UART_TX_GPIO = 4;
const uint MIDI_UART_RX_GPIO = 5;
#endif


static MIDI_UART_T *the_midi_uart;
static uint8_t dev_idx = TUSB_INDEX_INVALID_8;
static bool update_strings = false;

// Increase stack size when debug log is enabled
#define USBH_STACK_SIZE    (3*configMINIMAL_STACK_SIZE/2) * (CFG_TUSB_DEBUG ? 2 : 1)
#define MIDI_STACK_SIZE     (configMINIMAL_STACK_SIZE * (CFG_TUSB_DEBUG ? 2 : 1))


#if 0
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
    uint8_t nread = midi_uart_poll_rx_buffer(the_midi_uart, rx, sizeof(rx));
    if (nread > 0 && connected && tuh_midi_get_rx_cable_count(dev_idx) >= 1)
    {
        uint32_t nwritten = tuh_midi_stream_write(dev_idx, 0,rx, nread);
        if (nwritten != nread) {
            TU_LOG1("Warning: Dropped %lu bytes receiving from UART MIDI In\r\n", nread - nwritten);
        }
    }
}
#endif


// This top level thread process all usb events and invoke callbacks
void usb_host_task(void *param) {
  (void) param;

  // init device stack on configured roothub port
  // This should be called after scheduler/kernel is started.
  // Otherwise it could cause kernel issue since USB IRQ handler does use RTOS queue API.
  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_HOST,
    .speed = TUSB_SPEED_AUTO
  };
  tusb_init(0, &dev_init);

  board_init_after_tusb();

  // RTOS forever loop
  while (1) {
    // put this thread to waiting state until there is new events
    tuh_task();
  }
}


// unblock when the UART has posted data to send in the rx queue
void midi_tx_task(void* param) {
  (void) param;
  uint8_t val;
  while (1) {
    // block forever until the UART IRQ posts something
    xQueueReceive(the_midi_uart->midi_uart_rx_queue, &val, portMAX_DELAY);
    if (dev_idx == TUSB_INDEX_INVALID_8 || !tuh_midi_mounted(dev_idx))
        continue; // need to be connected to use the data
    // stream all MIDI bytes in the queue to USB
    do {
      tuh_midi_stream_write(dev_idx, 0, &val, 1);
    } while (xQueueReceive(the_midi_uart->midi_uart_rx_queue, &val, 0) == pdPASS);
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

static void on_midi_uart0_irq()
{
    on_midi_uart_irq(&midi_uart0);
}

static void on_midi_uart1_irq()
{
    on_midi_uart_irq(&midi_uart1);
}

static void on_midi_uart_irq(MIDI_UART_T *midi_uart)
{
    uint32_t status = uart_get_hw(midi_uart->midi_uart)->mis; // the reason this irq happened
    if (status & (UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS)) {
        // Got at least one byte; enqueue all in the FIFO; for less than 4 bytes, up to 1ms of latency introduced
        do {
            uint32_t dr = uart_get_hw(midi_uart->midi_uart)->dr;
            uint8_t val = dr & 0xff;
            if ((dr & !0xf00) == 0) {
                // then data read was OK.
                // If the queue is full, light an error LED
                if (xQueueSendFromISR(midi_uart->midi_uart_rx_queue, &val, NULL) != pdPASS) {
                    gpio_put(midi_uart->rx_overflow_gpio, true);
                }
            }
        } while ((uart_get_hw(midi_uart->midi_uart)->fr & UART_UARTFR_RXFE_BITS) == 0);
    }

    if (status & UART_UARTMIS_TXMIS_BITS) {
        // ready to transmit
        uint8_t val;
        uint32_t npopped = 0;
        while (uart_is_writable(midi_uart->midi_uart) && xQueueReceiveFromISR(midi_uart->midi_uart_tx_queue, &val, NULL) == pdPASS) {
          npopped++;
          uart_get_hw(midi_uart->midi_uart)->dr = val;
        }
        if (npopped < 4) {
            // Not enough data to trigger a transmit interrupt; disable the interrupt
            uart_set_irq_enables(midi_uart->midi_uart, true, false);
        }
    }
}

void *midi_uart_configure(uint8_t uartnum, uint8_t txgpio, uint8_t rxgpio)
{
    if (uartnum == 0) {
        midi_uart0.midi_uart = uart0;
        midi_uart0.midi_uart_irq = UART0_IRQ;
    }
    else {
        midi_uart1.midi_uart = uart1;
        midi_uart1.midi_uart_irq = UART1_IRQ;
    }

    MIDI_UART_T *midi_uart = NULL;
    if (uartnum == 1) {
        midi_uart = &midi_uart1;
        irq_set_exclusive_handler(midi_uart->midi_uart_irq, on_midi_uart1_irq);
    }
    else if (uartnum == 0) {
        midi_uart = &midi_uart0;
        irq_set_exclusive_handler(midi_uart->midi_uart_irq, on_midi_uart0_irq);
    }
    midi_uart->midi_uart_tx_gpio = txgpio;
    midi_uart->midi_uart_rx_gpio = rxgpio;
    midi_uart->tx_overflow_gpio = LED_TX_OVERFLOW_GPIO;
    midi_uart->rx_overflow_gpio = LED_RX_OVERFLOW_GPIO;
    midi_uart->midi_uart_tx_queue = xQueueCreate(MIDI_UART_QUEUE_LENGTH, 1);
    midi_uart->midi_uart_rx_queue = xQueueCreate(MIDI_UART_QUEUE_LENGTH, 1);
    // Set up the UART hardware
    uart_init(midi_uart->midi_uart, 31250);
    uart_set_format(midi_uart->midi_uart, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(midi_uart->midi_uart, false, false);
    uart_get_hw(midi_uart->midi_uart)->ifls = 0; // set FIFO interrupt levels to 1/8th full
    uart_set_fifo_enabled(midi_uart->midi_uart, true);
    
    uart_set_translate_crlf(midi_uart->midi_uart, false);

    gpio_set_function(midi_uart->midi_uart_tx_gpio, GPIO_FUNC_UART);
    gpio_set_function(midi_uart->midi_uart_rx_gpio, GPIO_FUNC_UART);
  
    // Set up data rate overflow indicator LEDs
    gpio_put(midi_uart->tx_overflow_gpio, false);
    gpio_put(midi_uart->rx_overflow_gpio, false);
    gpio_set_dir(midi_uart->tx_overflow_gpio, true);
    gpio_set_dir(midi_uart->rx_overflow_gpio, true);

    irq_set_enabled(midi_uart->midi_uart_irq, true);

    // enable the rx IRQ. No data to send yet.
    uart_set_irq_enables(midi_uart->midi_uart, true, false);
    return midi_uart;
}

int main() {
    bi_decl(bi_program_description("Provide a USB host interface for Serial Port MIDI."));
    bi_decl(bi_2pins_with_names(MIDI_UART_TX_GPIO, "MIDI UART TX", MIDI_UART_RX_GPIO, "MIDI UART RX"));
    TaskHandle_t usb_host;
    TaskHandle_t midi_tx;
    board_init();
    printf("Pico MIDI Host to MIDI UART Adapter\r\n");
    the_midi_uart = midi_uart_configure(MIDI_UART_NUM, MIDI_UART_TX_GPIO, MIDI_UART_RX_GPIO);
    //printf("Configured MIDI UART %u for 31250 baud\r\n", MIDI_UART_NUM);

    xTaskCreate(usb_host_task, "usbh", USBH_STACK_SIZE, NULL, configMAX_PRIORITIES-1, &usb_host);
    xTaskCreate(midi_tx_task, "midi_tx", USBH_STACK_SIZE, NULL, configMAX_PRIORITIES-2, &midi_tx);
    vTaskCoreAffinitySet(usb_host, 1);
    vTaskStartScheduler();
    return 0;
#if 0
    while (1) {
        tuh_task();

        blink_led();
        bool connected = dev_idx != TUSB_INDEX_INVALID_8 && tuh_midi_mounted(dev_idx);

        poll_midi_uart_rx(connected);
        if (connected)
            tuh_midi_write_flush(dev_idx);
        midi_uart_drain_tx_buffer(the_midi_uart);
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
#endif
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
    if (dev_idx != idx)
    {
        return;
    }
  uint32_t nread;
  uint8_t cable_num;
  uint8_t buffer[48];
  while ((nread = tuh_midi_stream_read(idx, &cable_num, buffer, sizeof(buffer))) != 0) {
#if 0
    printf("cable %u: ", cable_num);
    for (uint32_t idx = 0; idx < nread; idx++) {
      printf("%02x ", buffer[idx]);
    }
    printf("\r\n");
#endif
    if (cable_num == 0) {
        uint32_t idx = 0;
        // if the tx queue is empty, then try filling the TX FIFO first
        if (uxQueueMessagesWaiting(the_midi_uart->midi_uart_tx_queue) == 0) {
          while (idx < nread && uart_is_writable(the_midi_uart->midi_uart)) {
            uart_get_hw(the_midi_uart->midi_uart)->dr = buffer[idx++];
          }
          if (idx >= nread)
            return; // all data fit in the UART FIFO. No need for the TX IRQ
        }
        // If we got here, it is time to buffer in the queue
        while (idx < nread && uxQueueSpacesAvailable(the_midi_uart->midi_uart_tx_queue) != 0) {
          xQueueSend(the_midi_uart->midi_uart_tx_queue, buffer + idx++, 0);
        }
        // make sure the transmit interrupt is enabled
        uart_set_irq_enables(the_midi_uart->midi_uart, true, true);
        if (idx < nread) {
          // out of all buffer space. light the overflow indicator
          gpio_put(the_midi_uart->tx_overflow_gpio, true);
        }
    }
  }
#if 0
        if (num_bytes != 0)
        {
            uint8_t cable_num;
            uint8_t buffer[48];
            while (1) {
                uint32_t bytes_read = tuh_midi_stream_read(idx, &cable_num, buffer, sizeof(buffer));
                if (bytes_read == 0)
                    return;
                if (cable_num == 0) {
                    uint8_t npushed = midi_uart_write_tx_buffer(the_midi_uart,buffer,bytes_read);
                    if (npushed != bytes_read) {
                        TU_LOG1("Warning: Dropped %lu bytes sending to UART MIDI Out\r\n", bytes_read - npushed);
                    }
                }
            }
        }
    }
#endif
}

void tuh_midi_tx_cb(uint8_t idx, uint32_t num_bytes)
{
    (void)idx;
    (void)num_bytes;
}

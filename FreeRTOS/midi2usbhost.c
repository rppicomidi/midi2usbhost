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
#include "timers.h"
#include "pico/binary_info.h"
#include "bsp/board_api.h"
#include "tusb.h"

#ifndef MIDI_UART_QUEUE_LENGTH
#define MIDI_UART_QUEUE_LENGTH 128
#endif
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

// On-board LED mapping.
#ifndef LED_RX_OVERFLOW_GPIO
#define LED_RX_OVERFLOW_GPIO 2
#endif
#ifndef LED_TX_OVERFLOW_GPIO
#define LED_TX_OVERFLOW_GPIO 3
#endif
#ifndef LED_HOST_STATUS_GPIO
#define LED_HOST_STATUS_GPIO PICO_DEFAULT_LED_PIN
#endif
#ifndef OVERFLOW_TIMER_CLEAR_LAG_MS
#define OVERFLOW_TIMER_CLEAR_LAG_MS 1500
#endif
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
static TimerHandle_t host_blink_timer;
static TimerHandle_t tx_overflow_led_timer;
static TimerHandle_t rx_overflow_led_timer;

// Increase stack size when debug log is enabled
#define USBH_STACK_SIZE    (3*configMINIMAL_STACK_SIZE/2) * (CFG_TUSB_DEBUG ? 2 : 1)
#define MIDI_STACK_SIZE     (configMINIMAL_STACK_SIZE * (CFG_TUSB_DEBUG ? 2 : 1))

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
    xTimerStart(rx_overflow_led_timer, 0);
  }
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
            xTimerStartFromISR(tx_overflow_led_timer, NULL);
        }
    }
}

void overflow_led_timer_callback(TimerHandle_t timer)
{
  if (timer == tx_overflow_led_timer) {
    gpio_put(the_midi_uart->tx_overflow_gpio, false);
  }
  else {
    gpio_put(the_midi_uart->rx_overflow_gpio, false);
  }
}

void host_led_timer_callback(TimerHandle_t timer)
{
  (void)timer;
  gpio_xor_mask(1ul << LED_HOST_STATUS_GPIO);
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
    gpio_set_function(midi_uart->tx_overflow_gpio, GPIO_FUNC_SIO);
    gpio_set_function(midi_uart->rx_overflow_gpio, GPIO_FUNC_SIO);
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
    gpio_set_function(LED_HOST_STATUS_GPIO, GPIO_FUNC_SIO);
    gpio_put(LED_HOST_STATUS_GPIO, false);
    gpio_set_dir(LED_HOST_STATUS_GPIO, true);

    xTaskCreate(usb_host_task, "usbh", USBH_STACK_SIZE, NULL, configMAX_PRIORITIES-1, &usb_host);
    xTaskCreate(midi_tx_task, "midi_tx", USBH_STACK_SIZE, NULL, configMAX_PRIORITIES-2, &midi_tx);
    vTaskCoreAffinitySet(usb_host, 1);
    tx_overflow_led_timer = xTimerCreate("TxOvfLed", pdMS_TO_TICKS(OVERFLOW_TIMER_CLEAR_LAG_MS),false, (void*)0, overflow_led_timer_callback);
    rx_overflow_led_timer = xTimerCreate("RxOvfLed", pdMS_TO_TICKS(OVERFLOW_TIMER_CLEAR_LAG_MS),false, (void*)0, overflow_led_timer_callback);
    host_blink_timer = xTimerCreate("HostBlinkLed", pdMS_TO_TICKS(200), true, (void*)0, host_led_timer_callback);
    vTaskStartScheduler();
    return 0;
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+
// Invoked when device with MIDI interface is mounted
void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t* mount_cb_data)
{
  (void)mount_cb_data;
#if 0  
  printf("MIDI Device Index = %u, MIDI device address = %u, %u IN cables, OUT %u cables\r\n", idx,
      mount_cb_data->daddr, mount_cb_data->rx_cable_count, mount_cb_data->tx_cable_count);
#endif

  if (dev_idx == TUSB_INDEX_INVALID_8) {
    // then no MIDI device is currently connected
    dev_idx = idx;
    gpio_put(LED_HOST_STATUS_GPIO, true);
  }
  else {
    //printf("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled\r\n");
    xTimerStart(host_blink_timer,0);
  }
}

// Invoked when device with MIDI interface is un-mounted
void tuh_midi_umount_cb(uint8_t idx)
{
  if (idx == dev_idx) {
    dev_idx = TUSB_INDEX_INVALID_8;
    gpio_put(LED_HOST_STATUS_GPIO, false);
  }
  else {
    //printf("Unused MIDI Device Index  %u is unmounted\r\n", idx);
    xTimerStop(host_blink_timer,0);
    gpio_put(LED_HOST_STATUS_GPIO, dev_idx != TUSB_INDEX_INVALID_8);
  }
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t num_bytes)
{
    if (dev_idx != idx || num_bytes == 0) {
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
          if (idx >= nread) {
            // all data fit in the UART FIFO. No need for the TX IRQ
            return;
          }
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
}

void tuh_midi_tx_cb(uint8_t idx, uint32_t num_bytes)
{
    (void)idx;
    (void)num_bytes;
}

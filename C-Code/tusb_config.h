/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

// defined by compiler flags for flexibility
#ifndef CFG_TUSB_MCU
  #error CFG_TUSB_MCU must be defined
#endif
// RHPort max operational speed can defined by board.mk
#ifndef BOARD_TUH_MAX_SPEED
#define BOARD_TUH_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif
#ifdef RPPICOMIDI_ADAFRUIT_FEATHER_RP2040_USB_HOST
#define BOARD_TUD_RHPORT      0
#define RPPICOMIDI_CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE
// Use pico-pio-usb as host controller for raspberry rp2040
#define BOARD_TUH_RHPORT 1
#define CFG_TUSB_RHPORT1_MODE OPT_MODE_HOST
#define CFG_TUH_RPI_PIO_USB   1
// Fix mismatch between pico-sdk and TinyUSB build system; BOARD=pico-sdk breaks this
#ifndef PICO_DEFAULT_PIO_USB_DP_PIN
#define PICO_DEFAULT_PIO_USB_DP_PIN       16
#endif
#ifndef PICO_DEFAULT_PIO_USB_VBUSEN_PIN
#define PICO_DEFAULT_PIO_USB_VBUSEN_PIN   18
#endif
#ifndef PICO_DEFAULT_PIO_USB_VBUSEN_STATE
#define PICO_DEFAULT_PIO_USB_VBUSEN_STATE 1
#endif
#else
#if CFG_TUSB_MCU == OPT_MCU_LPC43XX || CFG_TUSB_MCU == OPT_MCU_LPC18XX || CFG_TUSB_MCU == OPT_MCU_MIMXRT10XX
  #define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_HOST | OPT_MODE_HIGH_SPEED)
#else
  #define CFG_TUSB_RHPORT0_MODE       OPT_MODE_HOST
#endif
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS                 OPT_OS_NONE
#endif
// CFG_TUSB_DEBUG is defined by compiler in DEBUG build
// #define CFG_TUSB_DEBUG           0

/* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
 * Tinyusb use follows macros to declare transferring memory so that they can be put
 * into those specific section.
 * e.g
 * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
 * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
 */
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN          __attribute__ ((aligned(4)))
#endif

//--------------------------------------------------------------------
// CONFIGURATION
//--------------------------------------------------------------------
// Enable Host stack, Default is max speed that hardware controller could support with on-chip PHY
#define CFG_TUH_ENABLED       1
#define CFG_TUH_MAX_SPEED     BOARD_TUH_MAX_SPEED

// Size of buffer to hold descriptors and other data used for enumeration
// See README.md Troubleshooting section for more details
#define CFG_TUH_ENUMERATION_BUFSIZE 512

#define CFG_TUH_HUB                 1 // Enable USB hubs
#define CFG_TUH_CDC                 0
#define CFG_TUH_HID                 0 // typical keyboard + mouse device can have 3-4 HID interfaces
#define CFG_TUH_MSC                 0
#define CFG_TUH_VENDOR              0

// max device support (excluding hub device)
#define CFG_TUH_DEVICE_MAX          (CFG_TUH_HUB ? 4 : 1) // hub typically has 4 ports
#define CFG_TUH_MIDI                (CFG_TUH_DEVICE_MAX)

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */

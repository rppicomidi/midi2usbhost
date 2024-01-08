/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 rppicomidi
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

/**
 * This demo program is designed to bridge the USB MIDI Host MIDI to
 * UART1 MIDI on an RP2040 for a single USB MIDI device connected to the USB
 * Host port. If the USB MIDI device has more than one virtual cable, it will
 * only bridge cable 0.
 *
 * This program works with a single USB MIDI device connected via a USB hub, but it
 * does not handle multiple USB MIDI devices connected at the same time.
 */
#ifndef USE_TINYUSB_HOST
#error "Please Select USB Stack: Adafruit TinyUSB Host"
#else
#warning "All Serial Monitor Output is on Serial1"
#endif
#include "EZ_USB_MIDI_HOST.h"

// Create the USB Host driver object
static Adafruit_USBH_Host USBHost;

USING_NAMESPACE_EZ_USB_MIDI_HOST
USING_NAMESPACE_MIDI

// Create the Hardware Serial object DINmidi with the default baud rate but the same interface settings as the USB host port
SerialMIDI<HardwareSerial, DefaultSerialSettings> serialDINmidi(Serial2);
MidiInterface<SerialMIDI<HardwareSerial, DefaultSerialSettings>, MidiHostSettings> DINmidi((SerialMIDI<HardwareSerial, DefaultSerialSettings>&)serialDINmidi);

// Create the USB MIDI Host Driver Object
static EZ_USB_MIDI_HOST USBmidi;

static uint8_t midiDevAddr = 0;

// DEFINE MESSAGE ROUTING
// Note that the MidiMessage data type from one MidiInterface type
// is not the same as the MidiMessage data type from another one
static void onUSBMIDIin(const MidiInterface<EZ_USB_MIDI_HOST_Transport>::MidiMessage& mes)
{
  //printf("usb:%02x %02x %02x %02x\r\n", mes.type, mes.data1, mes.data2, mes.channel);
  DINmidi.send(mes.type, mes.data1, mes.data2, mes.channel);
}

static void onDINMIDIin(const MidiInterface<HardwareSerial>::MidiMessage& mes)
{
  auto intf = USBmidi.getInterfaceFromDeviceAndCable(midiDevAddr, 0);
  if (intf != nullptr) {
    //printf("din:%02x %02x %02x %02x\r\n", mes.type, mes.data1, mes.data2, mes.channel);
    intf->send(mes.type, mes.data1, mes.data2, mes.channel);
  }
}

// CONNECTION MANAGEMENT
static void onMIDIconnect(uint8_t devAddr, uint8_t nInCables, uint8_t nOutCables)
{
  if (midiDevAddr != 0) {
    Serial1.println("Device Ignored. This program can only handle one USB MIDI device at a time.");
  }
  Serial1.printf("MIDI device at address %u has %u IN cables and %u OUT cables\r\n", devAddr, nInCables, nOutCables);
  midiDevAddr = devAddr;
  auto intf = USBmidi.getInterfaceFromDeviceAndCable(midiDevAddr, 0);
  if (intf == nullptr)
    return;
  intf->setHandleMessage(onUSBMIDIin);
  digitalWrite(LED_BUILTIN, HIGH); 
}

static void onMIDIdisconnect(uint8_t devAddr)
{
  Serial1.printf("MIDI device at address %u unplugged\r\n", devAddr);
  midiDevAddr = 0;
  digitalWrite(LED_BUILTIN, LOW); 
}

// Program initializations are in this function
void setup() {
  // Make sure the LED is off
  digitalWrite(LED_BUILTIN, LOW); 

  // Enable serial printf port
  Serial1.begin(115200);

  // Enable low level USB Host driver to use the RP2040 native USB port
  USBHost.begin(0);

  // Set up UART1 on pins GP4 and GP5, which are pins 6 & 7 on a Pico board
  Serial2.setTX(4);
  Serial2.setRX(5);
  // Tell DINmidi to route all incoming messages to the onDINMIDIin function
  DINmidi.setHandleMessage(onDINMIDIin);
  // Start the serial port MIDI in device
  DINmidi.begin(MIDI_CHANNEL_OMNI);

  // Initialize USB connection management
  USBmidi.setAppOnConnect(onMIDIconnect);
  USBmidi.setAppOnDisconnect(onMIDIdisconnect);

  delay(2000);
  Serial1.println("EZ_MIDI2USB_HOST");
}

// Program main loop is here
void loop() {
  // Update USB Host transfers
  USBHost.task();
  // Poll the USB host MIDI
  USBmidi.readAll();
  // Poll the serial port MIDI
  DINmidi.read();
  // Flush any writes after reading the serial port MIDI to USB packets
  USBmidi.writeFlushAll();
}

# midi2usbhost FreeRTOS-based C Code Software
This project routes serial port MIDI to virtual cable
0 of the connected USB MIDI device. If the USB MIDI device
supports other virtual cables, this project ignores MIDI Out
from the USB device and will not transmit to the USB device's
MIDI IN.

This project is an attempt to make the midi2usbhost software as
responsive as possible. It uses the SMP version of FreeRTOS to
hopefully improve TinyUSB responsiveness and it enables the UART FIFOs
to reduce the number of UART-related interrupts. It also eliminates
console printouts in favor of LED indicators. It uses one LED indicator
each thats lights if the serial port MIDI IN or MIDI OUT buffers
overflow. The main board LED shows USB host status: off means
no USB MIDI device enumerated; on means USB MIDI device enumerated;
blinking means more than one USB MIDI device have been connected
via a hub.

This project has been tested with version 2.2.0 of the `pico-sdk`.

# What is the most recent major change?
This is a new varient of the project.

## Set up your build environment
Sections 2 and 3 of the [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
describes how to set up your build environment for working using Microsoft
Visual Studio Code. Appendix C of the same document describes how to set
up the build environment manually for command line use. Please choose
a workflow you are comfortable with.

## Install FreeRTOS-Kernel source code
I recommend installing the FreeRTOS-Kernel source code at the same directory
level as your pico-sdk source code.

```
cd ${PICO_SDK_PATH}/..
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git
```
This code has been tested with the version of FreeRTOS-Kernel that has git
commit hash fcc665325c033065cb6fd89d18a92f2ca611495e.

## Update TinyUSB
Version 2.2.0 of the pico-sdk does not ship with the latest version of TinyUSB USB MIDI
Host support. I recommend that you update TinyUSB to version 0.20.0 (or the latest
release) to build this code. If you are using VS Code and installed
the pico-sdk per sections 2 and 3 of the [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf),
you will find TinyUSB installed in `${HOME}/.pico-sdk/sdk/2.1.1/lib/tinyusb`.
If you installed the pico-sdk per Appendix C of the same document, then
you will find TinyUSB installed in `${HOME}/pico/pico-sdk/lib/tinyusb`.

```
cd [the directory where TinyUSB is installed]
git checkout 0.20.0
git pull
```
I have also tested with this code with the TinyUSB master branch
commit hash 8b59e6234dfe34f4d0658cea43b338eb04efa3c5.

## Get the project code
Clone the midiusb2host project to a directory; if you manually installed the
`pico-sdk` per Appendix C of [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf),
it is best to clone to a directory at the same level as the `pico-sdk` directory.
If you are using version 2.1.1 of the `pico-sdk` installed from the Raspbery Pi Pico VS Code plugin, and you are building with VS Code, then where you clone the project files is less important.

```
cd ${PICO_SDK_PATH}/..
git clone --recurse-submodules https://github.com/rppicomidi/midi2usbhost.git
```
## Command Line Build (skip if you want to use Visual Studio Code)

Enter this series of commands (assumes you installed the `pico-sdk`, the `FreeRTOS-Kernel`
source and the `midid2usbhost` project in the `${HOME}/pico` directory)
and your board will be defined in `${MYBOARD}`. See the Hardware Variations
section for more information.
```
export PICO_BOARD=${MYBOARD}
export PICO_SDK_PATH=${HOME}/pico/pico-sdk
export FREERTOS_KERNEL_PATH=${HOME}/pico/FreeRTOS-Kernel
cd ${HOME}/pico/midi2usbhost/FreeRTOS
mkdir build
cd build
cmake ..
make
```
The build should complete with no errors. The build output is in the `build` directory you created in the steps above.

## Visual Studio Code Build
Run VS Code and import this project. It should import cleanly.
Build the code using the Raspberry Pi Pico plugin controls.
As long as the Pico SDK version is 2.2.0 or later and you
have updated the TinyUSB version as described above, and
you have installed the FreeRTOS-Kernel source, the
program should build with no errors.

## Run the code
You can load the program via the board's USB device port by loading
the `midi2usbhost.uf2` file found
in the build directory. You can load the program via the board's SWD port
by using a Debugprobe (formally called a picoprobe) or something similar
to load the `midi2usbhost.elf` file to the board using VS Code,
or by using openocd command
line tools.

# Hardware Variations
The values you can substitute for `${MYBOARD}` include all of the boards that
are supported in the `${PICO_SDK_PATH}/src/boards/include/boards` directory.

If you are targeting a board other than the Raspberry Pi Pico that does not have UART 1
available or does not have GPIO 4 or GPIO 5, which are the default pins for, you can
send data to CMake to properly target your hardware. There are 3 variables you can to set
- `MIDI_UART_NUM` can be 0 or 1 to depending on whether you use uart0 or uart1 for MIDI. The default value is 1.
- `MIDI_UART_TX_GPIO` defaults to 0 for uart0 and 4 for uart1. If you redefine it, make sure it is consistent with your choice of `MIDI_UART_NUM`
- `MIDI_UART_RX_GPIO` defaults to 1 for uart0 and 5 for uart1. If you redefine it, make sure it is consistent with your choice of `MIDI_UART_NUM`

Similarly, you can change what GPIO pins are used for indicator LEDs.
- `LED_RX_OVERFLOW_GPIO` defaults to GPIO 2
- `LED_TX_OVERFLOW_GPIO` defaults to GPIO 3
- `LED_HOST_STATUS_GPIO` defaults to the pico SDK's pre-defined `PICO_DEFAULT_LED_PIN`. Note that Pico W and Pico2 W boards do not
control the on-board LED by directly toggling a GPIO pin so `PICO_DEFAULT_LED_PIN` is undefined; if you target a Pico W board,
then this build will fail unless you redefine LED_HOST_STATUS_GPIO.

# The UART transmit and receive queues, and managing overflows
The overflow LEDs will remain illumated after the overflow conidition clears for `OVERFLOW_TIMER_CLEAR_LAG_MS` milliseconds.
The idea is to keep the LED on long enough so you notice the cause of the problem. If it is too short or long, you can change it.

The queues for buffering data to and from the UART are `MIDI_UART_QUEUE_LENGTH` bytes long. If you are getting a lot of
overflows, then you might want to make the queues longer. On the other hand, every 3 bytes in the queue represents about 1 millisecond.
If your USB device is sending so much data so quickly that it overwhelms the slow data rate of serial port MIDI, you might want to
consider modifying the code to filter and throttle the data rate. For example, a USB fader controller might overwhelm serial port
MIDI if you move more than one fader at a time too quickly. The fix might be to throw out some intermediate fader moves if
the transmit queue is filling up.

# Troubleshooting
Prior to TinyUSB adding native support for USB MIDI Host, this project
used the [usb_midi_host](https://github.com/rppicomidi/usb_midi_host) project
to provide the USB MIDI Host driver support. That project's README file
contained a lot of troubleshooting information for USB MIDI hosts.
Please see [this section](https://github.com/rppicomidi/usb_midi_host/blob/main/README.md#troubleshooting-configuration-and-design-details)
of the `usb_midi_host` project's `README.md` file for more
troublshooting hints.

If you are have trouble with an Arturia
Beatstep Pro, see [this bug](https://github.com/rppicomidi/usb_midi_host/issues/14).
The issue caused by a bug in the RP2040 native USB hardware. There is
a [pending pull request](https://github.com/hathach/tinyusb/pull/2814) in TinyUSB to resolve this issue.
Feel free to try this pull request.

If your project works for some USB MIDI devices and not others, one
thing to check is the size of buffer to hold USB descriptors and other
data used for USB enumeration. Look in the file `tusb_config.h` for
```
#define CFG_TUH_ENUMERATION_BUFSIZE 512
```
Very complex MIDI devices or USB Audio+MIDI devices like DSP guitar pedals
or MIDI workstation keyboards may have large USB configuration descriptors.
This project assumes 512 bytes is enough, but it may not be for your device.

To check if the descriptor size is the issue, use your development computer to
dump the USB descriptor for your device and then add up the wTotalLength field
values for each configuration in the descriptor.


For Linux and MacOS Homebrew, the command is lsusb -d [vid]:[pid] -v
For Windows, it is simplest to install a program like
[Thesycon USB Descriptor Dumper](https://www.thesycon.de/eng/usb_descriptordumper.shtml).

For example, this is the important information from `lsusb -d 0944:0117 -v`
from a Korg nanoKONTROL2:
```
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength       0x0053
    bNumInterfaces          1
    bConfigurationValue     1
    iConfiguration          0 
    bmAttributes         0x80     (Bus Powered)
```

Follow steps similar to those for opening and building the `usb_midi_host_example`
project. Be sure to select the board type so your project builds and runs correctly.on from the Thesycon USB Descriptor Dumper for
a Valeton NUX MG-400

```
0x01	bNumConfigurations

Device Qualifier Descriptor is not available. Error code: 0x0000001F


-------------------------
Configuration Descriptor:
-------------------------
0x09	bLength
0x02	bDescriptorType
0x0158	wTotalLength   (344 bytes)
0x04	bNumInterfaces
0x01	bConfigurationValue
0x00	iConfiguration
0xC0	bmAttributes   (Self-powered Device)
0x00	bMaxPower      (0 mA)
```
You can see that if `CFG_TUH_ENUMERATION_BUFSIZE` were 256 instead of 512,
the Korg nanoKONTROL2 would have no trouble enumerating but the Valeton
NUX MG-400 would fail because TinyUSB couldn't load the whole configuration
descriptor to memory.

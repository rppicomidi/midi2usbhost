# midi2usbhost C Code Software
This project is easiest to build with version 2.0 or later of the `pico-sdk`
with the included git submodule for the TinyUSB statck. This project relies
on [usb_midi_host](https://github.com/rppicomidi/usb_midi_host)
library to provide TinyUSB with a driver for the USB MIDI Host and the
[midi_uart_lib](https://github.com/rppicomidi/midi_uart_lib) and
[ring_buffer_lib](https://github.com/rppicomidi/ring_buffer_lib) libraries
to implement the DIN MIDI. These libraries are also git submodules.

## First step, build the `usb_midi_host_example` program
This program is only slightly different from the [usb_midi_host_example project](https://github.com/rppicomidi/usb_midi_host/tree/main/examples/C-code/usb_midi_host_example).
That program will run on the same hardware that this project uses. Please make
sure you can successfully build and run the `usb_midi_host_example` program
before building this project. The `usb_midi_host` project's
[README file](https://github.com/rppicomidi/usb_midi_host/blob/main/README.md)
has extensive instructions for setting up the build environment for various
versions of the `pico-sdk`.

## Get the project code
Clone the midiusb2host project to a directory; if you manually installed the
`pico-sdk` per Appendix C of [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf),
it is best to clone to a directory at the same level as the `pico-sdk` directory.
If you are using version 2.0 of the `pico-sdk` installed from the Raspbery Pi Pico VS Code plugin, and you are building with VS Code, then where you clone the project files
is less important.

```
cd ${PICO_SDK_PATH}/..
git clone --recurse-submodules https://github.com/rppicomidi/midi2usbhost.git
```
## Command Line Build (skip if you want to use Visual Studio Code)

Enter this series of commands (assumes you installed the `pico-sdk`
and the `midid2usbhost` project in the `${HOME}/foo` directory)

```
export PICO_SDK_PATH=${HOME}/foo/pico-sdk/
cd ${HOME}/foo/midi2usbhost/C-Code
mkdir build
cd build
cmake ..
make
```
If you were able to build `usb_midi_host_example` program, then this should work too.
The build should complete with no errors. The build output is in the `build` directory you created in the steps above.

## Set up and launch Visual Studio Code
Follow steps similar to those for opening and building the `usb_midi_host_example`
project.

## Run the code
You can load the program via the board's USB device port by loading
the `midi2usbhost.uf2` file found
in the build directory. You can load the program via the board's SWD port
by using a picoprobe or something similar
to load the `midi2usbhost.elf` file to the board using VS Code,
or by using openocd command
line tools.

Once the program is loaded, if all goes well, you should see the LED
on your Pico board blinking off and on once per second and your should see the following
in your terminal window:

```
Pico MIDI Host to MIDI UART Adapter
Configured MIDI UART 1 for 31250 baud
```
# Hardware Variations
If you are targeting a board other than the Raspberry Pi Pico that does not have UART 1
available or does not have GPIO 4 or GPIO 5, which are the default pins for, you can
send data to CMake to properly target your hardware. There are 3 variables you can to set

- `MIDI_UART_NUM` can be 0 or 1 to depending on whether you use uart0 or uart1 for MIDI. The default value is 1.
- `MIDI_UART_TX_GPIO` is the GPIO number (not the package pin number) of the UART's transmit pin. The default is 4. If you choose a different pin, make sure that the
UART you are using as set by `MIDI_UART_NUM` can use that pin for the UART TX function.
- `MIDI_UART_RX_GPIO` is the GPIO number (not the package pin number) of the UART's receiver pin. The default is 5. If you choose a different pin, make sure that the
UART you are using as set by `MIDI_UART_NUM` can use that pin for the UART RX function.

You can change these values by setting them as environment variables and then running
`cmake` or you can pass them directly on the `cmake` command line using the `-D` option.
For example, to use UART 0 on GPIO 12 & 13 as the MIDI UART you can build your code this way:
```
cd build
cmake -DMIDI_UART_NUM=0 -DMIDI_UART_TX_GPIO=12 -DDMIDI_UART_RX_GPIO=13 ..
make
```
or this way:
```
export MIDI_UART_NUM=0
export MIDI_UART_TX_GPIO=12
export MIDI_UART_RX_GPIO=13
cmake ..
make
```
I find the latter method simpler when I am using the VS Code workflow to build the code.

# Troubleshooting
Please see [this section](https://github.com/rppicomidi/usb_midi_host/blob/main/README.md#troubleshooting-configuration-and-design-details)
of the `usb_midi_host` project's `README.md` file for more
troublshooting hints.

If you are have trouble with an Arturia
Beatstep Pro, see [this bug](https://github.com/rppicomidi/usb_midi_host/issues/14).
The issue caused by a bug in the RP2040 native USB hardware. A robust
workaround requires rewrite of the TinyUSB host controller driver for
the RP2040 chip. The bug has a hack/patch you try around comment #39 that will
fix the worse issue for this class of bug.

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
    bmAttributes         0x80
      (Bus Powered)
    MaxPower              100mA
```
This is the important information from the Thesycon USB Descriptor Dumper for
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

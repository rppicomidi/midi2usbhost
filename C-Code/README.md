# midi2usbhost C Code Software
This project is easiest to build with version 2.1.1 or later of the `pico-sdk`.
This project relies on the [midi_uart_lib](https://github.com/rppicomidi/midi_uart_lib) and the
[ring_buffer_lib](https://github.com/rppicomidi/ring_buffer_lib) libraries
to implement the DIN MIDI.
These libraries are git submodules. The USB MIDI Host library is now
native to TinyUSB and is no longer a git submodule of this project.

# What is the most recent major change?
This project no longer depends on the the [usb_midi_host](https://github.com/rppicomidi/usb_midi_host)
project. When the TinyUSB project merged the USB MIDI Host pull request,
most of the API changed. The most significant change is that
the the TinyUSB MIDI Host API uses a device index instead of the USB device address
to reference MIDI devices. Also, the TinyUSB MIDI Host API changed some function names
and removed the device string functionality.

As an additional feature, this project now supports the Adafruit Feather RP2040
with USB A host board.

## Set up your build environment
Sections 2 and 3 of the [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
describes how to set up your build environment for working using Microsoft
Visual Studio Code. Appendix C of the same document describes how to set
up the build environment manually for command line use. Please choose
a workflow you are comfortable with.

## Update TinyUSB
As of this writing, the latest version of the pico-sdk (version 2.1.1) does not
ship with a version of TinyUSB that supports USB MIDI Host. You will need
to update TinyUSB to build this code. If you are using VS Code and installed
the pico-sdk per sections 2 and 3 of the [Getting Started Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf),
you will find TinyUSB installed in `${HOME}/.pico-sdk/sdk/2.1.1/lib/tinyusb`.
If you installed the pico-sdk per Appendix C of the same document, then
you will find TinyUSB installed in `${HOME}/pico/pico-sdk/lib/tinyusb`.

```
cd [the directory where TinyUSB is installed]
git checkout master
git pull
```
As of this writing, these steps will get you TinyUSB commit d3a9fee5cbd2490fa3d1e3976169e0c40e5a5e0c.

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

Enter this series of commands (assumes you installed the `pico-sdk`
and the `midid2usbhost` project in the `${HOME}/pico` directory)
and your board will be defined in `${MYBOARD}`. See the Hardware Variations
section for more information.
```
export PICO_BOARD=${MYBOARD}
export PICO_SDK_PATH=${HOME}/pico/pico-sdk/
cd ${HOME}/pico/midi2usbhost/C-Code
mkdir build
cd build
cmake -DPICO_BOARD=pico ..
make
```
The build should complete with no errors. The build output is in the `build` directory you created in the steps above.
Note that if your board is not a Raspberry Pi Pico, you should substitute your
board name for `pico` in the line `cmake -DPICO_BOARD=pico ..`

## Visual Studio Code Build
Run VS Code and import this project. It should import cleanly.
Build the code using the Raspberry Pi Pico plugin controls.
As long as the Pico SDK version is 2.1.1 or later and you
have updated the TinyUSB version as described above, the
program should build with no errors.

## Run the code
You can load the program via the board's USB device port by loading
the `midi2usbhost.uf2` file found
in the build directory. You can load the program via the board's SWD port
by using a Debugprobe (formally called a picoprobe) or something similar
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
The values you can substitute for `${MYBOARD}` include all of the boards that
are supported in the `${PICO_SDK_PATH}/src/boards/include/boards` directory.
As of this writing, only the Adafruit Feather RP2040 with USB A Host board
supports using the Pico PIO USB library and 2 GPIO pins for the USB MIDI host.
All other boards support using the built-in USB port for the USB Host port.

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

Note that if your board does not control and on-board LED by toggling a GPIO pin,
the this program will not control the on-board LED. The Pico W board is an example
of a board with this limitation.

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

# midi2usbhost C Code Software
This project depends on the usb\_midi\_host, midi\_uart\_lib, and ring\_buffer\_lib projects. They are included as git
submodules.

This project also uses the the Raspberry Pi Pico SDK and it uses the TinyUSB
library for the USB stack. At the time of this writing, the version of the
TinyUSB library that ships with the pico-sdk version 1.5.1 is not able to use
application USB Host drivers like usb\_midi\_host.  You will need a version of
TinyUSB from 15-Aug-2023 or later that calls `usbh_app_driver_get_cb()` to
install application USB Host drivers. See below for instructions on how to
update the TinyUSB library.

## Setting Up Your Build and Debug Environment
I am running Ubuntu Linux 22.04LTS on an old PC. I have Visual Studio Code (VS Code)
installed and went through the tutorial in Chapter 7 of [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
to make sure it was working first. I use a picoprobe for debugging, so I have openocd running in a terminal window.
I use minicom for the serial port terminal (make sure your linux account is in the dialup
group).

## Updating the TinyUSB library
The Pico SDK uses the main repository for TinyUSB as a git submodule. Earlier revisions of this project
replaced the TinyUSB library with a forked version. This is no longer necessary. You will need to check
the version of TinyUSB you are using and update it to a more recent version if it is older than 15-Aug-2023.

Assuming your pico-sdk is installed in `${PICO_SDK_PATH}`, then follow these steps to force your version
of TinyUSB to the latest version. The `git remote` command is only required if you previously changed
it per the instructions in older revisions of this project.

```
git remote set-url origin https://github.com/hathach/tinyusb.git
cd ${PICO_SDK_PATH}/lib/tinyusb
git checkout master
git pull
```

## Get the project code
Clone the midiusb2host project to a directory at the same level as the pico-sdk directory.

```
cd ${PICO_SDK_PATH}/..
git clone --recurse-submodules https://github.com/rppicomidi/midi2usbhost.git
```
## Command Line Build (skip if you want to use Visual Studio Code)

Enter this series of commands (assumes you installed the pico-sdk
and the midid2usbhost project in the $HOME/foo directory)

```
export PICO_SDK_PATH=$HOME/foo/pico-sdk/
cd $HOME/foo/midi2usbhost
mkdir build
cd build
cmake ..
make
```
The build should complete with no errors. The build output is in the build directory you created in the steps above.

## Set up and launch Visual Studio Code

Enter this series of commands 

```
cd midiusb2host
mkdir build
cd build
touch compile_commands.json
```

Run these once in a terminal before you launch VS Code. The first sets up the environment
and the second launches openOCD for use with the picoprobe

```
export PICO_SDK_PATH=$HOME/projects/pico/pico-sdk/
gnome-terminal -- openocd -f interface/cmsis-dap.cfg -c "adapter speed 5000" -f target/rp2040.cfg -s tcl
gnome-terminal -- minicom -D /dev/ttyACM0 -b 115200
```

Finally, launch VS Code

```
code
```

## Load the project
The first time you run the project, in VS Code, File->Open Folder... and select the midiusb2host folder. Click OK.

You will be prompted to set up the Kit. Choose GCC for arm-none-eabi [your version]

## Run the code
In VS Code, select Run->Start Debugging from the file menu. The first time, you will be
prompted to select the launch target. Select midi2usbhost.

VS Code debugger will load the code to your target Pico board and halt at main(). Press
the triangular run icon to start it running. If all goes well, you should see the LED
on your Pico board blinking off and on once per second and your should see the following
in your minicom terminal window:

```
Pico MIDI Host to MIDI UART Adapter
Configured MIDI UART 1 for 31250 baud
```
# Hardware Variations
If you are targeting a board other than the Raspberry Pi Pico that does not have UART 1
available or does not have GPIO 4 or GPIO 5, which are the default pins for, you can
data to CMake to properly target your hardware. There are 3 variables you can to set

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

# midi2usbhost
Use a Raspbery Pi Pico to add old school MIDI IN and MIDI OUT to MIDI devices with a USB B device port
# Disclaimers
This is a hardware and software project. The MIDI gadgets you will wire to this hardware will likely
cost a lot more than than the hardware for this project. Please double-check my design and test
your circuit and wiring before you connect it to anything expensive. I will take no responsibility
for burning up your favorite keyboard or sound module.

This projects uses commercial hardware for testing. I do not officially endorse any hardware. I have
not been paid by anyone to use this hardware. A carefully designed custom board would be a better
solution. The hardware combinations described here would almost certainly fail electrical and RF 
compliance testing. Use at your own risk.
# Hardware Dependencies
This project is designed to run on the RP2040 chip. It is tested on a 
[Raspberry Pi Pico board](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html),
but any board that provides access to the USB port and UART 1 will probably be fine.

The project uses the RP2040 UART 1 port for driving the old school MIDI IN and MIDI OUT ports. You can use whatever circuit
you would like for the current loop driver for MIDI OUT
and the optical isolator for MIDI IN. This project is tested using the
[MIDI Featherwing](https://learn.adafruit.com/adafruit-midi-featherwing) board. If you use this board, please
note that MIDI OUT is not isolated from your processor board UART 1 TX pin. Also note there are no
ferrite beads on MIDI IN and MIDI OUT connector pins, so radio frequency interference (RFI) may be
an issue with this circuit.

You will need to provide +5VDC to the VBus input. I use the VBUS pin from a picoprobe board.

If you use the Pico board, you will need to be able to connect your USB MIDI device to the Pico's micro USB connector.
I use the least expensive micro USB to full size USB A OTG adapter I could find. I won't link to one here. Web search
is your friend. Note that just using the adapter provides no current limit on the USB host VBus line, so please be
careful what you connect to this.

If you choose to use the same hardware I did, wire the boards together as follows.

```
USB C Breakout board VBus pin -> Pico board VBUS Pin 40
USB C Breakout board GND pin  -> Pico board GND Pin 38
Pico board GND Pin 8 -> MIDI Featherwing board GND Pin 4
Pico board 3.3V Pin 36 -> MIDI Featherwing board 3.3V Pin 2
Pico board UART1 TX Pin 6 -> MIDI Featherwing board Pin 15
Pico board UART1 RX Pin 7 -> MIDI Featherwing board Pin 14
```

A photo of my development setup using a second Pico board as a picoprobe is below. The Pico board
in the middle is running the MIDI adapter software. It is wired to the MIDI featherwing on
the left and has the Micro USB to USB A adapter on it. The picoprobe is providing VBUS power
and the serial port console. The picoprobe wiring is

```
picoprobe GND -> MIDI Pico GND
picoprobe GP2 -> MIDI Pico SWCLK
picoprobe GP3 -> MIDI Pico SWDIO
picoprobe GP4/UART1 TX -> MIDI Pico GP1/UART0 RX
picoprobe GP5/UART1 RX -> MIDI Pico GP0/UART0 TX
picoprobe VBUS -> Pico B VBUS
```
![*Pico USB MIDI Host Adapter with picoprobe on the right*](./docs/midiusb2host_dev.jpg)

# Software Dependencies
This project depends on the usb\_midi\_host, midi\_uart\_lib, and ring\_buffer\_lib projects. They are included as git
submodules.

This project also uses the the Raspberry Pi Pico SDK and it uses the TinyUSB library for the USB stack. At the time of this writing, the version of the
TinyUSB library that ships with the pico-sdk version 1.5.1 is not able to use
application USB Host drivers like usb\_midi\_host.  You will need a version of TinyUSB from 15-Aug-2023 or later that calls `usbh_app_driver_get_cb()` to
install application USB Host drivers. See below for instructions on how to
update the TinyUSB library.

# Setting Up Your Build and Debug Environment
I am running Ubuntu Linux 22.04LTS on an old PC. I have Visual Studio Code (VS Code)
installed and went
through the tutorial in Chapter 7 or [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) to make sure it was working
first. I use a picoprobe for debugging, so I have openocd running in a terminal window.
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
gnome-terminal -- openocd -f interface/picoprobe.cfg -f target/rp2040.cfg
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


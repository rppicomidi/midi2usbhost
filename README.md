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
Pico board GND Pin 40 -> MIDI Featherwing board GND Pin 4
Pico board VSYS Pin 38 -> MIDI Featherwing board 3.3V Pin 2
Pico board UART1 RX Pin 6 -> MIDI Featherwing board Pin 14
Pico board UART1 TX Pin 7 -> MIDI Featherwing board Pin 15
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
This project uses the the Raspberry Pi Pico SDK and it uses the tinyusb library for the USB stack. At the time
of this writing my pull request for supporting USB MIDI host is still pending, so please use my forked tinyusb
library until it is accepted.

This project also depends on the midi\_uart\_lib and ring\_buffer\_lib projects. They are included as git
submodules.
# Setting Up Your Build and Debug Environment
I am running Ubuntu Linux 20.04LTS on an old PC. I have Visual Studio Code (VS Code)
installed and went
through the tutorial in Chapter 7 or [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) to make sure it was working
first. I use a picoprobe for debugging, so I have openocd running in a terminal window.
I use minicom for the serial port terminal (make sure your linux account is in the dialup
group).

## Using the forked tinyusb library
The Pico SDK uses the main repository for tinyusb as a git submodule. Until the USB Host driver for MIDI is
incorporated in the main repository for tinyusb, you will need to use my forked version. This is how I do it.

1. If you have not already done so, follow the instructions for installing the Raspberry Pi Pico SDK in Chapter 2 of the 
[Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
document.
2. Create an "upstream" remote.
```
git remote add upstream https://github.com/hathach/tinyusb.git
```
3. Change the "origin" remote to point at my fork
```
git remote set-url origin https://github.com/rppicomidi/tinyusb.git
```
4. Get the code from my fork into your local repository
```
git fetch origin
```
5. Get the midihost branch code branch
```
git checkout -b midihost origin/midihost
```

## Get the project code
Clone the midiusb2host project to a directory at the same level as the pico-sdk directory.

```
cd [one directory above the pico-sdk directory]
git clone --recurse-submodules https://github.com/midiusb2host.git
```

## Set up and launch Visual Studio Code

This series of commands 

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


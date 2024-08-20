# midi2usbhost Arduino Sketch

# Hardware
This project uses the RP2040's USB hardware for the USB host port,
and it uses UART1 for the serial MIDI port. Debug prints appear on
the UART 0 port. See the [README.md Hardware Dependencies](https://github.com/rppicomidi/midi2usbhost#hardware-dependencies)
section for wiring and photos of the test hardware. Note that this
software is not designed to run on the Adafruit board with the built-in
USB A connector.

# License Notes
Code that I have written is released under the MIT License. Some libraries
the Arduino Sketch links to are released under other licenses. The
discussion [here](https://support.arduino.cc/hc/en-us/articles/4415094490770-Licensing-for-products-based-on-Arduino)
may be helpful.

# Software Build and Run
Make sure your Arduino IDE is set up to build software for RP2040 boards.
If you have never used Arduino to build software for the RP2040 chip,
you can find a good tutorial [here](https://learn.adafruit.com/adafruit-feather-rp2040-with-usb-type-a-host/arduino-ide-setup).
You should make sure your Board Manager has installed
verion 3.6.3 or later of the
[Earle Philhower Arduino Core](https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json).

Once you are able to build and run a simple blink example, please use
the Arduino IDE Library Manager to install the
[EZ_USB_MIDI_HOST](https://github.com/rppicomidi/EZ_USB_MIDI_HOST) library and all of its dependencies.

Use a web browser to navigate to the [midi2usbhost](https://github.com/rppicomidi/midi2usbhost)
project page. Click the `<> Code` Button and select `Download ZIP`. The browser will download
the file `midi2usbhost-main.zip`. Extract the ZIP file and copy the directory
`midi2usbhost/arduino/midi2usbhost` to your Sketch directory.

In the Arduino IDE, use `File->Open...` to load the `midi2usbhost/midi2usbhost.ino` file.

In the IDE Tools menu, select `USB Stack: "Adafruit USB Host"`.

## Picoprobe setup and run
Connect the picoprobe UART0 pins to the UART0 pins on target board and
connect the debug pins and ground to the 3 debug pins on the target board.
In the IDE Tools menu, select `Upload Method: "Picoprobe (CMSIS-DAP)"`.
In the IDE Tools menu, select `Debug Port: "Serial 1".
Connect the appropriate adapter to the target board USB port (e.g., for
a Pico board, a Micro USB to USB A adapter).

Finally press the Upload icon in the Arduino IDE.

The Sketch should compile and run. The Serial Monitor (115200 baud) should
display
```
midi2usbhost
```
Plug a USB MIDI device to your target board through the adapter you have.
The LED on the target board should illuminate to show the MIDI device has
enumerated successfully. Also he Serial Monitor should display information
about the MIDI device, and the serial MIDI to USB MIDI bridge should
operate now for MIDI IN and MIDI OUT.

## No Picoprobe setup and run
Unless you are using a picoprobe or other debug module to program the
target board, you need to use the target's USB port to program it.

Make sure you have a USB cable you can plug from your target board to the
computer running the Arduino IDE. The cable should not be connected to
the computer or the target board yet. Also make sure the target board is not
receiving power from any source. In the IDE Tools menu, select
`Upload Method: "Default (UF2)"`. Plug one end of the USB cable to the
computer, press the boot button on the target board, and plug the other
end of the USB cable to the target board. Your computer should mount
the target board as a drive called `RPI-RP2`. Let go of the board's boot
button. Click the IDE `Upload` button. The Sketch should compile and upload.
When upload is complete, disconnect the computer and connect your USB adapter
to the target's USB connector. If you wish, connect a USB to serial
port converter to the target board's UART0 pins. Apply 5V Vbus power to the
target board. If you have a USB to serial port converter on UART0, then
the Serial Monitor will display
```
midi2usbhost
```
Plug a USB MIDI device to your target board through the adapter you have.
The LED on the target board should illuminate to show the MIDI device has
enumerated successfully. Also he Serial Monitor should display information
about the MIDI device, and the serial MIDI to USB MIDI bridge should
operate now for MIDI IN and MIDI OUT.

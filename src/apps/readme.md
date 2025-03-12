# Firmwares for the STM32L452 processor

## Common

Library code used by multiple projects.

## 01-blinky-hal

Project showing that running the ARM works. The LEDs D61 and D62 are blinking and
are responding to the four input keys. This project uses the HAL library from ST.

## 02-test-everything-hal

Uses a RS232 port with 19200baud to allow controlling all connected hardware.

## 03-loader

__Main firmware__ intended to be progammed into the internal flash.
Provides a DFU device via USB to upload new programs.
Also writes a configuration in the external flash for the selected LCD type.
Can store and load programs from the external flash. Formatting can be done too.
Debug prints are available over the RS232 port with 19200baud.


## 04-coprocessor-uart-forward

If the coprocessor is running the 02-test-everything firmware, it prints data at 1200baud on the pins connected to the ARM.
With this firmware the data is forwared to the RS232 port with 19200baud and prints the data on the LCD too.

## 05-coprocessor-control

<img align="right" src="../../img/screenshot-battery-charged.png" alt="Screenshot after charging the battery">

If the coprocessor is running the 05-charger-with-spi firmware, this firmware allows reading out the charger state, resetting
the charger state and playing with the power off features.
Debug prints are available over the RS232 port with 19200baud.

## 06-usb-mass-storage

Connect the external SPI flash as USB mass storage device to the PC, allows reading and writing files on the disk.
As always, Debug prints are available over the RS232 port with 19200baud.

## 07-ntp-clock

<img align="right" src="../../img/screenshot-clock-big.png" alt="Screenshot of the big clock">

<img align="right" src="../../img/screenshot-clock-config.png" alt="Screenshot of the configuration">

Shows a big digital clock on the screen. Foreground and background color can be configured out of 256 colors.
The clock is synced over WIFI with an NTP server, therefore AP, password and NTP server can be set.
The backlight level and timezone can be set and the internal clock is calibrated automatically
when there are two syncs with enough time inbetween.
If a battery is connected, the clock continues to run while the device is off.
To allow updateing the GUI while the WIFI is served, FreeRTOS is used.
All settings are saved to the filesystem.
Debug prints are available over the RS232 port with 19200baud.
Also entering the AP and password is much easier over the RS232 port.

## 08-freertos-helloworld
Simple example how to use FreeRTOS.
Starts four threads, one for reading the serial port, one for resetting the watchdog, one for flashing the LEDs and one for reading the buttons.

## 09-gamebox

<img align="right" src="../../img/screenshot-gamebox.png" alt="Screenshot of the gamebox, playing tetris">

Port of an old project, originally written for an AVR with a red/green LED matrix.
Play Tetris, Snake, Four winns, Reversi, Race or Pong with just 16x16 pixels and 4 bit colors.
Also contains a demo.
Best used with an analogue joystick. But the input keys work too.
Tetris, Snake, Four winns, Reversi can also be played over the RS232 input, using w-a-s-d as cursor and space for enter.
The sourcecode is pretty old and still commented in german.
Running the gamebox on the AVR can be watched on [Youtube](https://www.youtube.com/watch?v=83r08iD9ZAA)
The connection of the analogue joystick to the STM32 is as follow:
<img align="right" src="09-gamebox/arm/image128x128.ppm" alt="Schematic for d-sub connection">

## 10-infrared-decoder

Prints the protocol, address, command and flags of many infrared remote controls on the LCD and
serial port.

## 11-adc-scope

<img align="right" src="../../img/screenshot-scope.png" alt="Screenshot of the scope">

Shows the analogue inputs on the display. Only supports 320x240 LCD.

Features:

- 200kHz sample rate (limit is the CPU checking the trigger condition, the ADC could be 20x faster)

- Select 3 Inputs out of all available ADC inputs

- Selectable X (time) and Y (voltage) scaling

- Selectable Y offset

- Trigger: Level, rising or falling edge. Normal or single shot.

- Max, min and average voltage analysis per channel

- Save screenshors and save/restore selected settings

Note: This is __not__ a fully featured oscilloscope.
It is intended to easy see what the AD values on the inputs are.
For example you can analyze the input signals if a joystick for 09-gamebox is connected and does not work as expected.
Actually the screenshot shows the signal while moving the two axis of the joystick.

Features __missing__ to be a real oscilloscope replacement:

- Selctable X offset - the trigger is always in the middle of the screen

- Y scaling and offset separated per channel

- X-Y mode

- Reasonable samplerate for signals above ~20kHz

- Defined input impendance

- Selectable input gain

- Support of negative input voltages

- Proper capture of every trigger event and displaying this as greyscale value

- Reasonable calibration. The internal oscillator might have +-2% error on the X axis

- Rotary encoders for a fast-to-use user interface. Four buttons are really limited.

- Proper analogue/digital separation on PCB

## 12-usb-cdc-loop

Ported USB cdc loop sample from [Libusb_stm32](https://github.com/dmitrystu/libusb_stm32).

After starting, an USB to serial converter is registered. All data transmitted are looped
back to the input. This allows testing the USB connection.


## 14-wav-player

Plays a normal uncompressed audio .wav file over one 8 bit PWM channel from the internal flash.
[Needs some external components](https://www.mikrocontroller.net/articles/Klangerzeugung#Lautsprecher) to be connected to port B, pin 4.
Supports:
- 8 and 16bit PCM
- mono and stereo
- 100Hz to 44200Hz samplerate

16bit and stereo files are automatically converted to 8 bit mono.
Other formats are not supported.
The .wav file can be transferred to the flash with the 06-usb-mass-storage project.

The audio quality can not be compared to a normal computer, it is more on the level of a telephone or a cheap toy doing some sounds.
The reason is the simple A/D conversion with a 128kHz PWM signal.

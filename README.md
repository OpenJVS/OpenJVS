# ModernJVS

ModernJVS is fork of OpenJVS, an emulator for I/O boards in arcade machines that use the JVS protocol. It requires a USB RS485 converter, or an official OpenJVS HAT.

Updated to use libgpiod, with backwards-compatible support for the now deprecated sysfs. Optimized code and new features, as well as support to the Raspberry Pi 5.

Updated using Github Copilot.

The following arcade boards are supported:

- Naomi 1/2
- Triforce
- Chihiro
- Hikaru
- Lindbergh
- Ringedge 1/2
- Namco System 22/23
- Namco System 2x6
- Taito Type X+
- Taito Type X2
- exA-Arcadia

## Installation

Installation is done from the git repository as follows, using RaspiOS Lite:

```
sudo apt install build-essential cmake git file libgpiod-dev
git clone https://github.com/dazzaXx/ModernJVS
make
sudo make install
```

If using DietPi:

```
sudo apt install build-essential cmake git file libgpiod-dev pkg-config
git clone https://github.com/dazzaXx/ModernJVS
make
sudo make install
```

## Supported Hardware

ModernJVS supports all Raspberry Pi models including:
- Raspberry Pi 1, 2, 3, 4
- Raspberry Pi 5 (with automatic GPIO chip detection)

The software automatically detects the correct GPIO chip for your Raspberry Pi model.

On games that require a sense line, the following has to be wired up:

```

|          GND   (BLACK) |-------------| GND           |                 |                        |
| ARCADE   A+    (GREEN) |-------------| A+  RS485 USB |-----------------| USB  RASPBERRY PI > 1  |
|          B-    (WHITE) |-------------| B-            |                 |                        |
|                        |                                               |                        |
|          SENSE (RED)   |----------+------------------------------------| GPIO 12                |
                                    |
                                    +---- (1kOhm Resistor or 4 Signal Diodes) ---- GND
```

A 1KOhm resistor or 4 signal diodes are known to work properly, the purpose of these is to create a 2.5 volt drop.

> Warning: A 1KOhm resistor will not work with the Triforce system, please use the 4 signal diodes for this purpose.

When buying a USB to RS485 dongle be sure to buy one with an FTDI chip inside. The CP2102 and other chips have been found to not work well.

I have tested CH340E USB RS485 converters personally and they work. ~dazzaXx

## Configuration

ModernJVS can be configured using the following command:
```
sudo nano /etc/modernjvs/config
```
Check the /etc/modernjvs/ios folder to see which I/O boards can be emulated and input the name of it on the EMULATE line.

By default it will emulate the Namco FCA1, as well as the debug level set to 1.


## Controller Deadzone Support

With version 4.6.2, configurable deadzone can now be changed in the config for each players controller. Only affects controllers with analog sticks as it's main use is to eliminate stick drift.


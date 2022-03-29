# OpenJVS

OpenJVS is an emulator for JVS Arcade IO boards which are used in many arcade systems to this day.

Questions can be asked in the discord channel: https://arcade.community. If it asks you to create an account, you can simply click anywhere away from the box (the dark area) and it'll let you in!

## Requirements

OpenJVS requires a USB RS485 dongle to communicate, or an OpenJVS Hat and supports the following hardware:

| Platform                        | Status      | Sense Line Required |
|---------------------------------|-------------|---------------------|
| Naomi 1                         | Working     | No                  |
| Naomi 2                         | Working     | Yes                 |
| Triforce                        | Mostly      | Yes                 |
| Chihiro                         | Working     | Yes                 |
| Lindbergh                       | Working     | Yes                 |
| Ringedge                        | Requires JVS Hat          | Yes                    |
| Ringedge 2                      | Requires JVS Hat          | Yes                    |
| Namco System 23 (Time Crisis 2) | Working     | Yes                 |
| Namco System 256                | Working     | Yes                 |
| Taito Type X+                   | Working     | Yes                 |
| Taito Type X2                   | Working     | Yes                 |
| exA-Arcadia                     | Working     | No                  |

On games that require a sense line, the following has to be wired up:

```

|          GND   (BLACK) |-------------| GND           |                 |                        |
| ARCADE   A+    (GREEN) |-------------| A+  RS485 USB |-----------------| USB  RASPBERRY PI > 1  |
|          B-    (WHITE) |-------------| B-            |                 |                        |
|                        |                                               |                        |
|          SENSE (RED)   |----------+------------------------------------| GPIO 12                |
                                    |
                                    +---- (1kOhm Resistor or 4 Signal Dioes) ---- GND
```

A 1KOhm resistor or 4 signal diodes are known to work properly, the purpose of these is to create a 2.5 volt drop.

When buying a USB to RS485 dongle be sure to buy one with an FTDI chip inside. The CP2102 and other chips have been found to not work well. 


## Installation and Running

To install OpenJVS follow the instructions below to install the required packages and make the program.

```
sudo apt install git cmake
git clone https://github.com/OpenJVS/OpenJVS
cd OpenJVS
make
```

To run locally (from inside the root directory):

```
sudo ./build/openjvs [optional outside mapping name]
```

To install globally and run (from inside the root directory):

```
make install
sudo openjvs [optional outside mapping name]
```

To make OpenJVS run at startup you can use:

```
sudo systemctl enable openjvs
sudo systemctl start openjvs
```

> Note: You must then stop OpenJVS running with `sudo systemctl stop openjvs` if you want to then run it locally for testing.

All input devices are enabled by default and must be disabled explicitly, to see what devices are available type:

```
sudo openjvs --list
```

You can then enable and disable the devices by running:

```
# To enable
sudo openjvs --enable sony-playstation-r-3-controller

# To disable
sudo openjvs --disable sony-playstation-r-3-controller
```

Devices are automatically used if there is no map for them. OpenJVS will attempt to detect if the device is a keyboard or joystick and map it according to the default mapping file for those device types.

If you'd like to stop the automatic detection, you can do this in the config file. If you'd like to stop an individual device being detected, you can create a blank mapping file for it in /etc/openjvs/devices.

Each new device is seen as a new player. For example if you plug 2 playstation controllers in, they will be mapped to PLAYER 1 and PLAYER 2. This means you should disable controllers you don't want to use, as they will take up player space.

## AimTrak

The aimtrak support was added by fred and bobbydilley. Aimtraks should be plug and play, remember the calibrate the gametrak via the guns calibration setting before calibrating the actual game via the game test menu. The aimtrak must be setup in the following way:

Please select 'Emulate mouse'.

### On Screen

- TRIG  -> Mouse Left
- LEFT  -> GP Button 1
- RIGHT -> GP Button 2

### Off Screen

- TRIG  -> Mouse Right
- LEFT  -> GP Button 3
- RIGHT -> GP Button 4

I also suggest to enable cal on Off Screen RIGHT only. This way you won't accidently put the gun into calibration mode when playing. Any other settings are up to the user.

## OpenJVS HAT

To support the new OpenJVS Hat some new features have been added

### Rotary

If you'd like to use the rotary selector on the OpenJVS hat to select your game, simply edit `/etc/openjvs/config` and set the following:

```
DEFAULT_GAME rotary
```

A file located at `/etc/openjvs/rotary` will allow you to list up to 16 games which can be selected using the rotary selector on the hat. The first game in the list coresponds to position 0, and the second to position 1 and so on.

Once you've made a change to the rotary position, you must restart OpenJVS for it to take effect.

## Mapping

The new mapping system is based on a two stage mapping process. The first stage will map your device to the controller seen below. The second stage will map the controller seen below to an actual game on an arcade system. This means that when you want to change controller, you only have to create one map file for your new controller, and likewise if you want to change game you only have to make one map file for your new game.

To create mapping you should become familiar with the program _evtest_ which can be installed with `sudo apt install evtest`. When you run it with `sudo evtest` it will show you all of the input devices you have connected, and if you select one it will list their capabilities and show you the input events they are producing when you press buttons.

### Real to Virtual Controller Mapping

Files for mapping your own controller to this virtual controller live in `/etc/openjvs/devices/` and should be named as the same name the controller comes up in when you run `sudo evtest` with the spaces replaced with `-` symbols.

The file consists of multiple lines with a FROM and TO mapping seperated with a space like below.

Note extra modifiers can be used on analogue channels:

- `REVERSE` can be added to the end of a line to reverse the direction of the input
- `SENSITIVITY 1.9` can be used to multiply the sensitivity of the devices analogue axis.

```
# Example Mapping File
# Author: Bobby Dilley

# Map the button section
# <FROM> <TO>
BTN_SOUTH CONTROLLER_ANALOGUE_A
BTN_NORTH CONTROLLER_ANALOGUE_C

# Map the analogue section
ABS_X CONTROLLER_ANALOGUE_X
ABS_Y CONTROLLER_ANALOGUE_Y REVERSE
ABS_Z CONTROLLER_ANALOGUE_R REVERSE SENSITIVITY 1.5
ABS_RZ CONTROLLER_ANALOGUE_L SENSITIVITY 0.9


# Map a HAT controller
ABS_HAT0X CONTROLLER_BUTTON_LEFT CONTROLLER_BUTTON_RIGHT
```

As above you can map the HAT controls which are sometimes used for DPADS. This should take controller button outputs on a single line. This can map any analogue channel into a digital one so analogue hats as well as thumb sticks can be converted into digitals!

You can also use this file to fix the player mapping of a certain controller using the `PLAYER` modifier, an example is below:

```
PLAYER 2
```

This would mean that any devices with that name will be assigned to PLAYER 2, and the standard player orders will not be effected by this device.

FROM keywords are selected from the list of linux input event keywords. These are the same as the ones shown when you run `sudo evtest`. TO keywords are selected from the pre defined virtual controller mapping keywords list. Below is the virtual controller that the mapping is based upon, along with the mapping keywords used to reference this controller.

```
          ____________________________              __
         / [__LT__]          [__RT__] \               |
        / [__ LB __]        [__ RB __] \              | Front Triggers
     __/________________________________\__         __|
    /                                  _   \          |
   /      /\           __             (C)   \         |
  /       ||      __  |TE|  __     _       _ \        | Main Pad
 |    <===DP===> |SE|      |ST|   (B) -|- (D) |       |
  \       ||    ___          ___       _     /        |
  /\      \/   /   \        /   \     (A)   /\      __|
 /  \________ | LS  | ____ |  RS | ________/  \       |
|         /  \ \___/ /    \ \___/ /  \         |      | Control Sticks
|        /    \_____/      \_____/    \        |    __|
|       /                              \       |
 \_____/                                \_____/

     |________|______|    |______|___________|
       D-Pad    Left       Right   Action Pad
               Stick       Stick

                 |_____________|
                    Menu Pad

MAPPING KEYWORD                         DIAGRAM LABEL                 PURPOSE
---------------                         -------------                 -------
CONTROLLER_BUTTON_TEST                  TE                            Test Button,
CONTROLLER_BUTTON_TILT                                                Tilt Button,
CONTROLLER_BUTTON_COIN                  RS CLICK                      Coin Button,
CONTROLLER_BUTTON_START                 ST                            Start Button,
CONTROLLER_BUTTON_SERVICE               SE                            Service Button,
CONTROLLER_BUTTON_UP                    DP UP                         Joystick Up Button,
CONTROLLER_BUTTON_DOWN                  DP DOWN                       Joystick Down Button,
CONTROLLER_BUTTON_LEFT                  DP LEFT                       Joystick Left Button,
CONTROLLER_BUTTON_LEFT_BUMPER           LB                            Gear Down Button,
CONTROLLER_BUTTON_RIGHT                 DP RIGHT                      Joystick Right Button,
CONTROLLER_BUTTON_RIGHT_BUMPER          RB                            Gear Up Button,
CONTROLLER_BUTTON_A,                    A                             Button 1 / Trigger Button,
CONTROLLER_BUTTON_B,                    B                             Button 2 / Screen In / Out,
CONTROLLER_BUTTON_C,                    C                             Button 3 / Action Button 1,
CONTROLLER_BUTTON_D,                    D                             Button 4 / View Change Button,
CONTROLLER_BUTTON_E,                    LS CLICK                      Button 5,
CONTROLLER_BUTTON_F,                                                  Button 6,
CONTROLLER_BUTTON_G,                                                  Button 7,
CONTROLLER_BUTTON_H,                                                  Button 8,
CONTROLLER_BUTTON_I,                                                  Button 9,
CONTROLLER_BUTTON_J,                                                  Button 10,
CONTROLLER_ANALOGUE_X,                  LS X                          Analogue Joystick X / Steering Wheel,
CONTROLLER_ANALOGUE_Y,                  LS Y                          Analogue Joystick Y / Aeroplane Pitch,
CONTROLLER_ANALOGUE_Z,                  RS X                          Analogue Joystick Z,
CONTROLLER_ANALOGUE_R,                  RT                            Accelerator,
CONTROLLER_ANALOGUE_L,                  LT                            Breaks,
CONTROLLER_ANALOGUE_T,                  RS Y                          Analogue Joystick T,
```

### Virtual Controller to Game Mapping

Files for mapping your own controller to this virtual controller live in `/etc/openjvs/games/` and should be named either by their function such as `generic-driving` or by a specific game name `outrun`.

This file will be selected using the `DEFAULT_MAPPING` config keyword, or by a parameter passed to the program `sudo openjvs outrun2`.

The file consists of multiple lines with a `CONTROLLER_INPUT CONTROLLER_PLAYER ARCADE_INPUT ARCADE_PLAYER` mapping as shown below. Note `REVERSE` can be added to the end of a line to reverse the direction of the input.
Note a secondary `BUTTON_*` can be added to the end of the line as a secondary output which will be enabled with the first. This allows the emulating of H shifters in games like Wangan Midnight Maximum Tune.

```
# Example Mapping File
# Author: Bobby Dilley

# Map the button section
# <CONTROLLER_INPUT> <CONTROLLER_PLAYER> <ARCADE_INPUT> <ARCADE_PLAYER>
CONTROLLER_BUTTON_A CONTROLLER_1 BUTTON_1 PLAYER_1
CONTROLLER_BUTTON_B CONTROLLER_1 BUTTON_2 PLAYER_1

# Player Two
CONTROLLER_BUTTON_A CONTROLLER_2 BUTTON_1 PLAYER_2
CONTROLLER_BUTTON_B CONTROLLER_2 BUTTON_2 PLAYER_2

# Map the analogue section
CONTROLLER_ANALOGUE_X CONTROLLER_1 ANALOGUE_0
CONTROLLER_ANALOGUE_Y CONTROLLER_1 ANALOGUE_1

```

You can also now map an Analogue to a digital button using the DIGITAL modifier at the start of the line:

```
DIGITAL CONTROLLER_ANALOGUE_X CONTROLLER_1 BUTTON_1 PLAYER_1
```

The list of ARCADE_INPUT keywords are listed below:

```
MAPPING KEYWORD
---------------
BUTTON_TEST,
BUTTON_TILT,
BUTTON_START,
BUTTON_SERVICE,
BUTTON_UP,
BUTTON_DOWN,
BUTTON_LEFT,
BUTTON_RIGHT,
BUTTON_1,
BUTTON_2,
BUTTON_3,
BUTTON_4,
BUTTON_5,
BUTTON_6,
BUTTON_7,
BUTTON_8,
BUTTON_9,
BUTTON_10,
ANALOGUE_1,
ANALOGUE_2,
ANALOGUE_3,
ANALOGUE_4,
ANALOGUE_5,
ANALOGUE_6,
ROTARY_1,
ROTARY_2,
ROTARY_3,
ROTARY_4,
ROTARY_5,
ROTARY_6,
```

Other mapping files can be included, so for example if a game is almost exactly the same as a generic mapping with one small difference, the generic mapping can be included and the one small difference added.

```
# Example Outrun 2 Mapping File
# Author: Bobby Dilley

# Set the IO to emulate
EMULATE sega-type-3

# Include the generic driving file
INCLUDE generic-driving

# Make the additions
CONTROLLER_BUTTON_RIGHT_BUMPER CONTROLLER_1 BUTTON_UP PLAYER_2
CONTROLLER_BUTTON_LEFT_BUMPER CONTROLLER_1 BUTTON_DOWN PLAYER_2

```

As well as this the IO that should be emulated can be specified in the map file with the `EMULATE` keyword, these IOs are defined in the `/etc/openjvs/ios` directory.

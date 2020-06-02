# OpenJVS

OpenJVS is an emulator for JVS Arcade IO boards which are used in many arcade systems to this day.

Questions can be asked in the discord channel: https://discord.gg/aJAR9N2. If it asks you to create an account, you can simply click anywhere away from the box (the dark area) and it'll let you in!

## Requirements

OpenJVS requires a USB RS485 dongle to communicate, and supports the following hardware:

- Sega Naomi 1 (Without Sense Line)
- Sega Naomi 2
- Sega Triforce
- Sega Chihiro
- Sega Lindbergh
- Namco System 256

On games that require a sense line, the following has to be wired up:

A list of RS485 dongles and comments are below:

- FTDI - Always seems to work well
- CP2102 - Worked for me, but a lot of people are having issues.
- Other - Don't get them.

## Installation and Running

To install OpenJVS3 follow the instructions below to install the required packages and make the program.

```
sudo apt install git cmake
git clone https://github.com/bobbydilley/OpenJVS3
cd OpenJVS3
mkdir build && cd build
cmake ..
make
```

To run locally (from inside the build directory):

```
sudo ./openjvs [optional outside mapping name]
```

To install for globally and run (from inside the build directory):

```
cpack
sudo dpkg --install *.deb
sudo openjvs [optional outside mapping name]
```

All input devices are disabled by default and must be enabled explicitly, to see what devices are available type:

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

Each new device is seen as a new player. For example if you plug 2 playstation controllers in, they will be mapped to PLAYER 1 and PLAYER 2. This means you should disable controllers you don't want to use, as they will take up player space.

## Mapping

The new mapping system is based on a two stage mapping process. The first stage will map your device to the controller seen below. The second stage will map the controller seen below to an actual game on an arcade system. This means that when you want to change controller, you only have to create one map file for your new controller, and likewise if you want to change game you only have to make one map file for your new game.

To create mapping you should become familiar with the program _evtest_ which can be installed with `sudo apt install evtest`. When you run it with `sudo evtest` it will show you all of the input devices you have connected, and if you select one it will list their capabilities and show you the input events they are producing when you press buttons.

### Real to Virtual Controller Mapping

Files for mapping your own controller to this virtual controller live in `/etc/openjvs/devices/` and should be named as the same name the controller comes up in when you run `sudo evtest` with the spaces replaced with `-` symbols.

The file consists of multiple lines with a FROM and TO mapping seperated with a space like below. Note `REVERSE` can be added to the end of a line to reverse the direction of the input.

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


# Map a HAT controller
ABS_HAT0X CONTROLLER_BUTTON_LEFT CONTROLLER_BUTTON_RIGHT
```

As above you can map the HAT controls which are sometimes used for DPADS. This should take controller button outputs on a single line.

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
CONTROLLER_ANALOGUE_X ANALOGUE_0
CONTROLLER_ANALOGUE_Y ANALOGUE_1

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
EMULATE SEGA_TYPE_3

# Include the generic driving file
INCLUDE generic-driving

# Make the additions
CONTROLLER_BUTTON_RIGHT_BUMPER CONTROLLER_1 BUTTON_UP PLAYER_2
CONTROLLER_BUTTON_LEFT_BUMPER CONTROLLER_1 BUTTON_DOWN PLAYER_2

```

As well as this the IO that should be emulated can be specified in the map file with the `EMULATE` keyword, these IOs are created in `input.h` and are from the following list.

```
IO TO EMULATE
-------------
SEGA_TYPE_1_IO // Not yet done
SEGA_TYPE_2_IO // Not yet done
SEGA_TYPE_3_IO
NAMCO_JYU_IO
NAMCO_FCA_IO // Not yet done
OPENJVS_MEGA_IO // Not Yet Done
```

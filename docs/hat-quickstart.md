# OpenJVS Hat Quick Start Guide

This guide will help you setup OpenJVS with your Pi Hat.

## Jumpers

If you are using a Pi 4, you can have JVS / Serial and Bluetooth all running together. For this the jumpers must be aligned to the LEFT.

If you are using a Pi 2 or 3, you can only run JVS and cannot run Serial or onboard Bluetooth at the same time. The jumpers should be aligned to the RIGHT.

## Installation

Download the Raspberry Pi Imager from https://www.raspberrypi.org/software/. This will download a program available for Ubuntu, Mac and Windows.

Insert your SD card, and run the Imager. For the OS, select Other Raspberry Pi OS, and then select Raspberry Pi OS Lite. Then select the SD card you want to write to, remembering this will remove everything already on the SD.

When it asks if you want to "apply OS customization settings" click on the "Edit Settings" button. Here, you should check the "Set Hostname" box, then enter the Username and Password you will use as your Linux user. As the last step on the 'General' tab, set up the Wireless LAN settings, where you will need to add your ssid and password as well as country code.

Finally, click on the 'Services' tab and then check the "Enable SSH" box. The password and user will be the ones you set up on the previous tab.

More information can be found here: https://www.raspberrypi.org/documentation/configuration/wireless/headless.md.

Once the image has burnt, safely remove the card and then re-insert it into your computer.

Next you need to modify config.txt, adding the following to the bottom of the file.

If you're on a Pi 3 add:

```
dtoverlay=disable-bt
```

If you're on a Pi 4 add:

```
dtoverlay=uart3
dtoverlay=uart4
```

Next you must modify cmdline.txt

Remove the initial serial console line, from `console` up to the first space. It should look similar to the following line:

```
console=serial0,115200
```

Safely remove the SD card, place it into the Pi and then boot it.

You now need to find the Pi's IP Address. You can either do this by looking on your router's web page, or by downloading a network scanning application. The fastest way is to make a ping to the Hostname you set on the OS Settings.

```
ping raspberrypi.local
```

You should see the IP address in the ping command response. Use it to SSH into the Pi with the credentials you set on the OS Settings.

```
ssh <user>@<ip_address>
```

You should now be logged in and you should be able to see a prompt.

If you're using a Pi3, there's more to do to disable bluetooth. Run the following:

```
sudo systemctl disable hciuart
```

Now you need to install the required dependencies

```
sudo apt install git cmake evtest
```

Clone openjvs 

```
git clone https://github.com/openjvs/openjvs
```

Make and install OpenJVS

```
cd openjvs
make
sudo make install
```

Now you need to configure OpenJVS. Use your favourite text editor to edit `/etc/openjvs/config`.

```
sudo nano /etc/openjvs/config
```

Set default game to rotary, this will tell OpenJVS to use the rotary controller.
```
DEFAULT_GAME rotary
```

Set the sense line type to 2

```
SENSE_LINE_TYPE 2
```

Now you need to tell OpenJVS how to communicate with JVS.

If you're on a Pi4:

```
DEVICE_PATH /dev/ttyAMA1
```

If you're on a Pi3:

```
DEVICE_PATH /dev/ttyAMA0
```

Save the file and exit the editor.

Finally make OpenJVS start on boot

```
sudo systemctl enable openjvs
sudo systemctl start openjvs
```

And if you want you can view the OpenJVS Logs

```
sudo journalctl -u openjvs
```

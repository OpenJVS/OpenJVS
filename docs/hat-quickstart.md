# OpenJVS Hat Quick Start Guide

## Jumpers

If you are using a Pi4, you can have JVS / Serial and Bluetooth all running together. For this the jumpers must be aligned to the LEFT.

If you are using a Pi2 or 3, you can only run JVS and cannot run Serial or onboard Bluetooth at the same time. The jumpers should be aligned to the RIGHT.

## Installation

### OS Install

Download the [Raspberry Pi Imager](https://www.raspberrypi.org/software/). This will download a program available for Ubuntu, Mac and Windows.

Insert your SD card, and run the Imager. For the OS, select Other Raspberry Pi OS, and then select Raspberry Pi OS Lite. Then select the SD card you want to write to, remembering this will remove everything already on the SD.

When it asks if you want to "apply OS customization settings" click on the "Edit Settings" button. Here, you should check the "Set Hostname" box, then enter the Username and Password you will use as your Linux user.

As the last step on the "General" tab, set up the Wireless LAN settings, where you will need to add your ssid and password as well as country code.

Finally, click on the 'Services' tab and then check the "Enable SSH" box. The password and user will be the ones you set up on the previous tab.

> Note: If you have any trouble or for some reason you are using an older Pi OS version you can find more information can be found in the [Raspberry Pi Config Documentation Page](https://www.raspberrypi.org/documentation/configuration/wireless/headless.md).

Once the image has burnt, safely remove the card and then re-insert it into your computer.

Next you need to modify `config.txt`, adding the following to the bottom of the file.

If you're on a **Pi2 or 3** add:

```text
dtoverlay=disable-bt
```

If you're on a **Pi4** add:

```text
dtoverlay=uart3
dtoverlay=uart4
```

Next you must modify `cmdline.txt`

Remove the initial serial console line, from `console` up to the first space. It should look similar to the following line:

```text
console=serial0,115200
```

Safely remove the SD card, place it into the Pi and then boot it.

### Configuring the Pi & Installing OpenJVS

You now need to find the Pi's IP Address. You can either do this by looking on your router's web page, or by downloading a network scanning application. The fastest way is to make a ping to the Hostname you set on the OS Settings.

```bash
ping raspberrypi.local
```

You should see the IP address in the ping command response. Use it to SSH into the Pi with the credentials you set on the OS Settings.

```bash
ssh <user>@<ip_address>
```

You should now be logged in and you should be able to see a prompt.

If you're using a Pi3, there's more to do to disable bluetooth. Run the following:

```bash
sudo systemctl disable hciuart
```

Now you need to install the required dependencies

```bash
sudo apt install git cmake evtest
```

Clone openjvs

```bash
git clone https://github.com/openjvs/openjvs
```

Make and install OpenJVS

```bash
cd openjvs
make
sudo make install
```

### Configuring OpenJVS

Let's configure OpenJVS. To tell OpenJVS how to communicate with JVS, run the following command:

```bash
ls /dev/ttyAMA*
```

This command should list the serial ports available to your Raspberry Pi. The number can change depending on your Pi's model and OS (Usually there is one port available for the Pi3 and two for the Pi4). Pick the lowest ttyAMA port that appears and write it down, as we're gonna use it later.

Now, let's use your favourite text editor to edit `/etc/openjvs/config`.

```bash
sudo nano /etc/openjvs/config
```

Set default game to `rotary`, this will tell OpenJVS to use the rotary controller.

```text
DEFAULT_GAME rotary
```

Set the sense line type to `2`.

```text
SENSE_LINE_TYPE 2
```

And now set the Device Path with the number we found when listing the serial peripherals.

```text
DEVICE_PATH /dev/ttyAMA<number>
```

Save the file and exit the editor.

Finally, we can make OpenJVS start on boot.

```bash
sudo systemctl enable openjvs
sudo systemctl start openjvs
```

> Note: You must then stop OpenJVS running with `sudo systemctl stop openjvs` if you want to then run it locally for testing.

And if you want you can view the OpenJVS Logs.

```bash
sudo journalctl -u openjvs
```

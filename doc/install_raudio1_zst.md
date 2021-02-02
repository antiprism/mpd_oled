# Install instructions for rAudio 1 (RuneAudio fork)

These instructions are for installing mpd_oled with a binary package on
rAudio 1.

The binary package should be the best option for most people, but if you
would like to build and install the current mpd_oled repository code then see
[Install instructions for rAudio 1 using source](install_raudio1_source.md).

## Base system

Install [rAudio](https://github.com/rern/rAudio-1/).

Ensure a command line prompt is available for entering the commands
below (e.g. use SSH, default username 'root', default password 'ros').

## System settings

Configure your system to enable I2C or SPI, depending on how your OLED
is connected.

### I2C
I use a cheap 4 pin I2C SSH1106 display with a Raspberry Pi Zero. It is
[wired like this](wiring_i2c.png).
In /etc/modules-load.d/raspberrypi.conf I have the line `i2c-dev`.
```
nano /etc/modules-load.d/raspberrypi.conf
```

In /boot/config.txt I have the line `dtparam=i2c_arm=on`.
The I2C bus speed on your system may be too slow for a reasonable screen
refresh. Set a higher bus speed by adding
the following line `dtparam=i2c_arm_baudrate=400000` to
/boot/config.txt, or try a higher value for a higher screen
refresh (I use 800000 with a 25 FPS screen refresh)
```
sudo nano /boot/config.txt
```
Restart the Pi after making any system configuration changes.

### SPI
I use a cheap 7 pin SPI SSH1106 display with a Raspberry Pi Zero. It is
[wired like this](wiring_spi.png).
In /boot/config.txt I have the line `dtparam=spi=on`.
```
sudo nano /boot/config.txt
```
Restart the Pi after making any system configuration changes.

### Set the time zone
If, when running mpd_oled, the clock does not display the local time then
you may need to set the system time zone. Either set it in the UI
**Settings / System / Environment / Timezone**, or find your timezone in the
list printed by the first command below, and edit the second command to
include your timezone
```
timedatectl list-timezones
timedatectl set-timezone Canada/Eastern
```

## Install the mpd_oled package

This will download and install the most recent mpd_oled binary package
(the second command might take a couple of minutes to run)
```
wget -N http://pitastic.com/mpd_oled/packages/mpd_oled_raudio_install_latest.sh
sudo bash mpd_oled_raudio_install_latest.sh
```

## Configure a copy of the playing audio

You may wish to [test the display](#configure-mpd_oled) before
following the next instructions.

*The next instructions configure MPD to make a*
*copy of its output to a named pipe.*
*This works reliably, but has the disadvantage that the spectrum*
*only works when the audio is played through MPD, like music files,*
*web radio and DLNA streaming. Creating a copy of the audio for all*
*audio sources is harder, and may be unreliable -- see the thread on*
*[using mpd_oled with Spotify and Airplay](https://github.com/antiprism/mpd_oled/issues/4)*

The MPD audio output will be copied to a named pipe, where Cava can
read it and calculate the spectrum. This is configured in /etc/mpd.conf.
This file cannot be edited directly, as it is managed by rAudio, but
the UI will allow us to include some custom configuration in a
separate file. First, copy the configuration file (the destination
name is preserved from previous instructions)

```
cp mpd_oled_fifo.conf /home/your-extra-mpd.conf
```
Now, in the UI go to **Settings / MPD / Options / User's custom settings**
and click on the slider. A window will open with two boxes to enter
custom settings. In the top box, add the line
```
include "/home/your-extra-mpd.conf"
```
Click on OK.

## Configure mpd_oled

*Note: The program can be run without the audio copy enabled, in*
*which case the spectrum analyser area will be blank*

The OLED type MUST be specified with -o from the following list:
    1 - Adafruit (SSD1306, SSD1309) SPI 128x64,
    3 - Adafruit (SSD1306, SSD1309) I2C 128x64,
    4 - Seeed I2C 128x64,
    6 - SH1106 (SSH1106) I2C 128x64.
    7 - SH1106 (SSH1106) SPI 128x64.

An example command, for a generic I2C SH1106 display (OLED type 6) with
a display of 10 bars and a gap of 1 pixel between bars and a framerate
of 20Hz is
```
sudo mpd_oled_service_edit -o 6 -b 10 -g 1 -f 20
```

**For I2C OLEDs** (mpd_oled -o 3, 4 or 6) you may need to specify the I2C
address, find this by running, e.g. `sudo i2cdetect -y 1` and then specify
the address with option -a, e.g. `sudo mpd_oled_service_edit -o6 -a 3d ...`.
If you have a reset pin connected, specify the GPIO number with option -r,
e.g. `sudo mpd_oled_service_edit -o6 -r 24 ...`. Specify the I2C bus number,
if not 1, with option -B, e.g. `sudo mpd_oled_service_edit -o6 -B 0 ...`

**For, SPI OLEDs** (option -o 1 or 7), you may need to specify your reset pin
GPIO number (option -r, default 25), DC pin GPIO number (option -D,
default 24) or CS value (option -S, default 0).

If your display is upside down, you can rotate it 180 degrees with option '-R'.

Once the display is working, play some music and check the spectrum display
is working and is synchronised with the music. If there are no bars then the
audio copy may not have been configured correctly. If the bars seem jerky
or not synchronized with the music then reduce the values of -b and/or -f.

If you run `sudo mpd_oled_service_edit` without options the service
file will open in an editor, allowing the full service file to be
changed, and not just the mpd_oled options.

If the mpd_oled options are valid the display will be started after
the editor is closed, and will also be configured to start a boot

Check the program is working correctly by looking at the display while
the player is stopped, paused and playing music.


### Extra commands to control the service

A few selected commands that can be used to control the service
```
sudo systemctl enable mpd_oled    # start mpd_oled at boot
sudo systemctl disable mpd_oled   # don't start mpd_oled at boot
sudo systemctl start mpd_oled     # start mpd_oled now
sudo systemctl stop mpd_oled      # stop mpd_oled now
sudo systemctl status mpd_oled    # report the status of the service
```

## Uninstall

Uninstall with
```
sudo pacman -Rs mpd-oled
```

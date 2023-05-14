# Install instructions for Moode 8

These instructions are for installing mpd_oled with a binary package on
Moode 8 (valid for both 64-bit and legacy 32-bit kernel architecture).

The binary package should be the best option for most people, but if you
would like to build and install the current mpd_oled repository code then see
[Install instructions for Moode 8 using source](install_moode8_source.md).

## Base system

Install [Moode](http://moodeaudio.org/). Ensure a command line prompt is
available for entering the commands below (e.g. use SSH, enable in the Moode UI
at **Configure / System / Local Services / SSH term server**, log in with
default username 'pi', default password 'moodeaudio').

## System settings

Configure your system to enable I2C or SPI, depending on how your OLED
is connected.

### I2C
I use a cheap 4 pin I2C SSH1106 display with a Raspberry Pi Zero. It is
[wired like this](wiring_i2c.png).

In /etc/modules I have the line `i2c-dev`
```
sudo nano /etc/modules
```

In /boot/config.txt I have the line `dtparam=i2c_arm=on`.
The I2C bus speed on your system may be too slow for a reasonable screen
refresh. Set a higher bus speed by adding
the following line `dtparam=i2c_arm_baudrate=400000` to
/boot/config.txt, or try a higher value for a higher screen refresh
(I use `dtparam=i2c_arm_baudrate=800000` with a 25 FPS screen refresh)
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

## Install the mpd_oled package

This will download and install the most recent mpd_oled binary package
```
wget -N http://pitastic.com/mpd_oled/packages/mpd_oled_moode_install_latest.sh
sudo bash mpd_oled_moode_install_latest.sh
```

## Configure song status and time zone

In the Moode UI, set your timezone at **Moode / Configure / System**.

Enable the Moode metadata file, which includes information about the current
song. The setting will either be at
**Moode / Configure / System / Local Services / Metadata file**
or
**Moode / Configure / Audio / MPD Options / Metadata file**.

## Configure a copy of the playing audio for the spectrum display

Go to **Configure / Audio / ALSA Options**, set ALSA Loopback to ON and
click on SET.

## Configure mpd_oled and set to run at boot

The mpd_oled program can be run with `sudo mpd_oled_service_edit` (plus
options), and this also sets up mpd_oled with the same options as a service
to be run at boot. Rerunning `sudo mpd_oled_service_edit` with different
options will stop the current running mpd_oled and start it again with
the new options. (Test commands can also be run with `mpd_oled` (plus
options), and stopped with Ctrl-C, but ensure that no other copy of
mpd_oled is running).

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
sudo mpd_oled_service_edit -o 6 -b 10 -g 1 -f 20 -c alsa,plughw:Loopback,1
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

Uninstall the mpd_oled service (just the service, not the mpd_oled programs
or documentation), with
```
sudo mpd_oled_service_uninstall
```

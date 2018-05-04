# Install instructions for RuneAudio

## Base system

Install [RuneAudio](http://www.runeaudio.com/). Ensure a command line prompt is
available for entering the commands below (e.g. use SSH, default username
'root', default password 'rune').

## Build and install cava

mpd_oled uses Cava, a bar spectrum audio visualizer, to calculate the spectrum
   
   <https://github.com/karlstav/cava>

The commands to download, build and install Cava on RuneAudio are as follows
```
pacman -S autoconf automake clang make libtool fftw alsa-lib
git clone https://github.com/karlstav/cava
cd cava
./autogen.sh
CC=clang ./configure
make
make install
```

## System settings

Configure your system to enable I2C or SPI, depending on how your OLED
is connected.

I use a cheap 4 pin I2C SSH1106 display with a Raspberry Pi Zero. It is
[wired like this](https://www.14core.com/wp-content/uploads/2016/11/Raspberry-Pi-2-OLED_Screen-WIring-Diagram-Monocrome-I2C.jpg). In /boot/config.txt I
have the line `dtparam=i2c_arm=on`. In /etc/modules-load.d/raspberrypi.conf
I have the lines `i2c-dev` and `i2c-bcm2708`.

The I2C bus speed on your system may be too slow for a reasonable screen
refresh. Set a higher bus speed by adding the
following line to /boot/config.txt (or try a higher value for a higher
screen refresh, I use 800000 with a 25 FPS screen refresh)
```
dtparam=i2c_arm_baudrate=400000
```
Then restart the Pi.

If the mpd_oled clock does not display the local time then you may need
to set the system time zone. Find your timezone in the list printed by the
first command below, and edit the second command to include your timezone
```
timedatectl list-timezones
timedatectl set-timezone Canada/Eastern
```

## Build and install mpd_oled

Install the packages needed to build the program
```
pacman -S i2c-tools lm_sensors
```
Clone the source repository and change to the source directory
```
git clone https://github.com/antiprism/mpd_oled
cd mpd_oled
```

The MPD audio output needs to be copied to a named pipe, where Cava can
read it and calculate the spectrum. This is configured in /etc/mpd.conf,
however, RuneAudio regenerates this file in response to various events and
so the RuneAudio code must be changed to insert the section when the
file is generated. The following commands install the patch program,
copy the FIFO configuration file to /usr/local/etc/mpd_oled_fifo.conf,
and patch the RuneAudio code to append this file when generating
/etc/mpd.conf. (Note: if, for any reason, regeneration of /etc/mpd.conf
has been disabled (for example, if it has been set immutable) then edit
the file directly and append the contents of mpd_oled_fifo.conf.)

```
pacman -S patch
cp mpd_oled_fifo.conf /usr/local/etc/
patch -d/ -p0 -N < runeaudio_mpd_fifo.patch
```
Reboot the machine from RuneUI, then log back in and change to the
mpd_oled source directory, e.g.
```
cd mpd_oled
```
If you ever want to make any changes to the FIFO configuration,
for example you might want to change buffer_time to help synchronise
the spectrum display with the audio on your system,
then modify /usr/local/etc/mpd_oled_fifo.conf and restart MPD,
by going to the RuneUI MPD Configuration page and clicking on
"SAVE AND APPLY" in the volume control section.

Now build mpd_oled
```
CC=clang CXX=clang++ PLAYER=RUNEAUDIO make
```
Check the mpd_oled program works correctly by running it while playing music.
The OLED type MUST be specified with -o from the following list:
    1 - Adafruit SPI 128x64,
    3 - Adafruit I2C 128x64,
    4 - Seeed I2C 128x64,
    6 - SH1106 I2C 128x64.

E.g. the command for a generic I2C SH1106 display (OLED type 6) with
a display of 10 bars and a gap of 1 pixel between bars and a framerate
of 15Hz is
```
./mpd_oled -o 6 -b 10 -g 1 -f 15
```
For I2C OLEDs you may need to specify the I2C address, find this by running,
e.g. `i2cdetect -y 1` and specify the address with mpd_oled -a,
e.g. `./mpd_oled -o6 -a 3d ...`. If you have a reset pin connected, specify
the GPIO number with mpd_oled -r, e.g. `./mpd_oled -o6 -r 24 ...`. (For, SPI
OLEDs, edit display.cc to include your connection details, if this works
out I will provide options for these parameters.)

Once the display is working, edit the file mpd_oled.service to include
your OLED type number with the mpd_oled command, and any other options.
```
nano mpd_oled.service
```

Then run
```
bash install.sh
```
This will copy the program to /usr/local/bin and add a systemd service
to run it and start it running. You can start, stop, disable, etc the
service with commands like
```
systemctl start mpd_oled
```
If you wish to change mpd_oled parameters later then edit mpd_oled.service
to include the changes and rerun install.sh.


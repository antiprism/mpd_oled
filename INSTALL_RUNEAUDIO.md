# Install instructions for RuneAudio

## Base system

Install [RuneAudio](http://www.runeaudio.com/). These instructions are
compatible with [RuneAudio 0.5b-10 downloaded from this message](http://www.runeaudio.com/forum/runeaudio-0-5-beta-for-all-raspberry-pi-models-t6532.html).
Check the RuneAudio forum [mpd_oled thread](http://www.runeaudio.com/forum/mpd-vol-oled-status-and-spectrum-display-for-raspberry-pi-t6338.html) for additional information.

Ensure a command line prompt is available for entering the commands
below (e.g. use SSH, default username 'root', default password 'rune').

Install all the packages needed to build and run mpd_oled
```
pacman -Syy
pacman -Sy autoconf automake clang make libtool fftw alsa-lib llvm-libs glibc gcc i2c-tools
```

## System settings

Configure your system to enable I2C or SPI, depending on how your OLED
is connected.

### I2C
I use a cheap 4 pin I2C SSH1106 display with a Raspberry Pi Zero. It is
[wired like this](wiring_i2c.png).
In /boot/config.txt I have the line `dtparam=i2c_arm=on`.
In /etc/modules I have the line `i2c-dev`.

The I2C bus speed on your system may be too slow for a reasonable screen
refresh. Set a higher bus speed by adding the following line to
/boot/config.txt, or try a higher value for a higher screen
refresh (I use 800000 with a 25 FPS screen refresh)
```
dtparam=i2c_arm_baudrate=400000
```
Restart the Pi after making any system configuration changes.

### SPI
I use a cheap 7 pin SPI SSH1106 display with a Raspberry Pi Zero. It is
[wired like this](wiring_spi.png).
In /boot/config.txt I have the line `dtparam=spi=on`.

Restart the Pi after making any system configuration changes.

### Set time zone
If, when running mpd_oled, the clock does not display the local time then
you may need
to set the system time zone. Find your timezone in the list printed by the
first command below, and edit the second command to include your timezone
```
timedatectl list-timezones
timedatectl set-timezone Canada/Eastern
```

## Build and install cava

mpd_oled uses Cava, a bar spectrum audio visualizer, to calculate the spectrum
   
   <https://github.com/karlstav/cava>

The commands to download, build and install Cava on RuneAudio are as follows
```
cd ~
git clone https://github.com/karlstav/cava
cd cava
./autogen.sh
CC=clang ./configure
make
make install
```

## Build and install mpd_oled

Clone the mpd_oled source repository, change to the source directory and build
the program
```
cd ~
git clone https://github.com/antiprism/mpd_oled
cd mpd_oled
CC=clang CXX=clang++ PLAYER=RUNEAUDIO make
```

The MPD audio output needs to be copied to a named pipe, where Cava can
read it and calculate the spectrum. This is configured in /etc/mpd.conf,
but any changes to this file may be overwritten by RuneAudio. Instead,
copy the configuration to a special location and RuneAudio will
incorporate it into /etc/mpd.conf.
```
cp mpd_oled_fifo.conf /home/your-extra-mpd.conf
```
Update /etc/mpd.conf now by going to the RuneUI MPD Configuration
page, and clicking on "SAVE AND APPLY" in the volume control section.

If you ever want to make any changes to the FIFO configuration,
then modify /home/your-extra-mpd.conf and restart MPD,
by going to the RuneUI MPD Configuration page and clicking on
"SAVE AND APPLY" in the volume control section.

Check the program works correctly by running it while playing music.
The OLED type MUST be specified with -o from the following list:
    1 - Adafruit SPI 128x64,
    3 - Adafruit I2C 128x64,
    4 - Seeed I2C 128x64,
    6 - SH1106 I2C 128x64.
    7 - SH1106 SPI 128x64.

E.g. the command for a generic I2C SH1106 display (OLED type 6) with
a display of 10 bars and a gap of 1 pixel between bars and a framerate
of 20Hz is
```
sudo ./mpd_oled -o 6 -b 10 -g 1 -f 20
```
The program can be stopped by pressing Control-C.

For I2C OLEDs (mpd_oled -o 3, 4 or 6) you may need to specify the I2C address,
find this by running,
e.g. `sudo i2cdetect -y 1` and specify the address with mpd_oled -a,
e.g. `./mpd_oled -o6 -a 3d ...`. If you have a reset pin connected, specify
the GPIO number with mpd_oled -r, e.g. `mpd_oled -o6 -r 24 ...`. Specify
the I2C bus number, if not 1, with mpd_oled -B, e.g. `mpd_oled -o6 -B 0 ...`

For, SPI OLEDs (mpd_oled -o 1 or 7), you may need to specify your reset pin
GPIO number (mpd_oled -r, default 25), DC pin GPIO number (mpd_oled -D,
default 24) or CS value (mpd_oled -S, default 0).

If your display is upside down, you can rotate it 180 degrees with option '-R'.

Once the display is working, play some music and check the spectrum display
is working and is synchronised with the music. If there are no bars then the
audio copy may not have been configured correctly. If the bars seem jerky
or not synchronized with the music then reduce the values of -b and/or -f.

When you have found some suitable options then edit the file mpd_oled.service
to include your OLED type number and other options as part of the mpd_oled
command.
```
nano mpd_oled.service
```

Then run
```
sudo bash install.sh
```
This will copy the program to /usr/local/bin and add a systemd service
to run it and start it running. You can start, stop, disable, etc the
service with commands like
```
sudo systemctl start mpd_oled
```
If you wish to change mpd_oled parameters later then edit mpd_oled.service
to include the changes and rerun install.sh.

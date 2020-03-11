# Install instructions for Moode

These instructions are compatible with Moode 6 (and Moode 5) using a
32-bit kernel architecture.

## Base system

Install [Moode](http://moodeaudio.org/). Ensure a command line prompt is
available for entering the commands below (e.g. use SSH, default username
'pi', default password 'moodeaudio')

## Build and install cava

mpd_oled uses Cava, a bar spectrum audio visualizer, to calculate the spectrum
   
   <https://github.com/karlstav/cava>

The commands to download, build and install Cava are as follows.
```
sudo apt-get update
sudo apt-get install libfftw3-dev libasound2-dev
git clone https://github.com/karlstav/cava
cd cava
./autogen.sh
./configure
make
sudo make install
```

## System settings

Configure your system to enable I2C or SPI, depending on how your OLED
is connected.

### I2C
I use a cheap 4 pin I2C SSH1106 display with a Raspberry Pi Zero. It is
[wired like this](http://www.raspberrypirobotics.com/wp-content/uploads/2018/09/Interfacing-circuit-diagram-of-OLED-Display-with-Raspberry-Pi.png).
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
[wired like this](http://www.raspberrypirobotics.com/wp-content/uploads/2018/03/Interfacing-OLED-Display-with-Raspberry-Pi-circuit-hardware.jpg).
In /boot/userconfig.txt (or use /boot/config.txt for Volumio versions before
2.673) I have the line `dtparam=spi=on`.

Restart the Pi after making any system configuration changes.


## Build and install mpd_oled

Install the packages needed to build the program
```
sudo apt install libi2c-dev i2c-tools lm-sensors
```
Clone the source repository and change to the source directory
```
git clone https://github.com/antiprism/mpd_oled
cd mpd_oled
```

The MPD audio output needs to be copied to a named pipe, where Cava can
read it and calculate the spectrum. This is configured in /etc/mpd.conf.
However, Moode regenerates this file, and also disables all but a single MPD
output, in response to various events, and so the Moode code must be changed.
The current version, Moode 6, includes technical measures to disallow
code changes; run the following commands to disable them
```
sqlite3 /var/local/www/db/moode-sqlite3.db "DROP TRIGGER ro_columns"
sqlite3 /var/local/www/db/moode-sqlite3.db "UPDATE cfg_hash SET ACTION = 'warning' WHERE PARAM = '/var/www/command/worker.php'"
sqlite3 /var/local/www/db/moode-sqlite3.db "UPDATE cfg_hash SET ACTION = 'warning' WHERE PARAM = '/var/www/inc/playerlib.php'"
```

The following commands copy the FIFO configuration file to
/usr/local/etc/mpd_oled_fifo.conf and patch the Moode source code. (Note 1:
a Moode system update may overwrite the patched code, in which case, repeat
the next instructions, and possibly also the previous instructions.
Note 2: if, for any reason, regeneration of
/etc/mpd.conf has been disabled (for example, if it has been set immutable)
then edit the file directly and append the contents of mpd_oled_fifo.conf.)
(for older versions Moode, 6.4.0 and earlier, in the second command below
specify the patch file moode_mpd_fifo_old.patch).

```
sudo cp mpd_oled_fifo.conf /usr/local/etc/
sudo patch -d/ -p0 -N < moode_mpd_fifo.patch
```

Enable the Moode metadata file
```
sqlite3 /var/local/www/db/moode-sqlite3.db "update cfg_system set value=1 where param='extmeta'" && mpc add ""

```
Go to the Moode UI and set your timezone at "Moode" / "Configure" / "System",
then (essential) reboot the machine.

Log back into the machine and change to the mpd_oled source directory, e.g.
```
cd mpd_oled
```
If you ever want to make any changes to the FIFO configuration,
then modify /usr/local/etc/mpd_oled_fifo.conf and restart MPD,
by going to the Moode UI Audio Config page and clicking on
"RESTART" in the MPD section (for Moode 5, restart MPD now).

Now build mpd_oled (for Moode 5, build with a plain `make`)
```
PLAYER=MOODE make
```
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

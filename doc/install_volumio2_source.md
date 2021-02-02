# Install instructions for Volumio 2 using source

These instructions are for installing mpd_oled using source on
Volumio 2.

A binary package is also available, but may not include the very
latest code, see
[Install instructions for Volumio 2](install_volumio2_deb.md).


## Base system

Install [Volumio](https://volumio.org/). Ensure a command line prompt is
available for entering the commands below (e.g.
[use SSH](https://volumio.github.io/docs/User_Manual/SSH.html).)

## Install all dependencies

Install all the packages needed to build and run cava and mpd_oled
```
sudo apt update
sudo apt install build-essential git-core autoconf make libtool libfftw3-dev libmpdclient-dev libi2c-dev i2c-tools lm-sensors
```

## Build and install cava

mpd_oled uses Cava, a bar spectrum audio visualizer, to calculate the spectrum
   
   <https://github.com/karlstav/cava>

If you have Cava installed (try running `cava -h`), there is no need
to install Cava again, but to use the installled version you must use
`mpd_oled -k ...`.

Download, build and install Cava. These commands build a reduced
feature-set executable called `mpd_oled_cava`.
```
git clone https://github.com/karlstav/cava
cd cava
./autogen.sh
./configure --disable-input-portaudio --disable-input-sndio --disable-output-ncurses --disable-input-pulse --program-prefix=mpd_oled_
make
sudo make install-strip
```

## Build and install mpd_oled

Download, build and install mpd_oled.
```
cd ..   # if you are still in the cava source directory
git clone https://github.com/antiprism/mpd_oled
cd mpd_oled
./bootstrap
CPPFLAGS="-W -Wall -Wno-psabi" ./configure
make
sudo make install-strip
```


## System settings

Configure your system to enable I2C or SPI, depending on how your OLED
is connected.

### I2C
I use a cheap 4 pin I2C SSH1106 display with a Raspberry Pi Zero. It is
[wired like this](wiring_i2c.png).
In /etc/modules I have the line `i2c-dev` (included by default).
In /boot/config.txt I have the line `dtparam=i2c_arm=on` (included by default).

The I2C bus speed on your system may be too slow for a reasonable screen
refresh. Set a higher bus speed by adding the line
`dtparam=i2c_arm_baudrate=400000` to
/boot/userconfig.txt (or use /boot/config.txt for Volumio versions before
2.673), or try a higher value for a higher screen refresh (I use 800000 with a
25 FPS screen refresh)
```
sudo nano /boot/userconfig.txt
```

Restart the Pi after making any system configuration changes.

### SPI
I use a cheap 7 pin SPI SSH1106 display with a Raspberry Pi Zero. It is
[wired like this](wiring_spi.png).
In /boot/userconfig.txt (or use /boot/config.txt for Volumio versions before
2.673) I have the line `dtparam=spi=on`.
```
sudo nano /boot/userconfig.txt
```

Restart the Pi after making any system configuration changes.

## Configure a copy of the playing audio

*The next instruction configure MPD to make a copy of its output to a*
*named pipe, where Cava can read it and calculate the spectrum.*
*This works reliably, but has two disadvantages: the configuration*
*involves changing a Volumio system file, which must be undone*
*if Volumio is to be updated (see below); the spectrum*
*only works when the audio is played through MPD, like music files,*
*web radio and DLNA streaming. Creating a copy of the audio for all*
*audio sources is harder, and may be unreliable -- see the thread on*
*[using mpd_oled with Spotify and Airplay](https://github.com/antiprism/mpd_oled/issues/4)*

Configure MPD to copy its audio output to a named pipe
```
sudo mpd_oled_volumio_mpd_conf_install
```

**Note:** after running this command the next Volumio update will fail
with a *system integrity check* error. The change can be undone by running
`sudo mpd_oled_volumio_mpd_conf_uninstall`, then after the Volumio update
run `sudo mpd_oled_volumio_mpd_conf_install` to re-enable the audio copy.

## Set the time zone

If the mpd_oled clock does not display the local time then you may need
to set the system time zone. Set this in the UI, or run the following
command for a console based application where you can specify your location
```
sudo dpkg-reconfigure tzdata
```

## Configure mpd_oled

*Note: The program can be run without the audio copy enabled, in*
*which case the spectrum analyser area will be blank*

The OLED type MUST be specified with -o from the following list:
    1 - Adafruit (SSD1306, SSD1309) SPI 128x64,
    3 - Adafruit (SSD1306, SSD1309) I2C 128x64,
    4 - Seeed I2C 128x64,
    6 - SH1106 (SSH1106) I2C 128x64.
    7 - SH1106 (SSH1106) SPI 128x64.

E.g. the command for a generic I2C SH1106 display (OLED type 6) with
a display of 10 bars and a gap of 1 pixel between bars and a framerate
of 20Hz is
```
sudo mpd_oled -o 6 -b 10 -g 1 -f 20
```
The program can be stopped by pressing Control-C.

For I2C OLEDs (mpd_oled -o 3, 4 or 6) you may need to specify
the I2C address, find this by running,
e.g. `sudo i2cdetect -y 1` and then specify the address with mpd_oled -a,
e.g. `mpd_oled -o6 -a 3d ...`.
If you have a reset pin connected, specify
the GPIO number with mpd_oled -r, e.g. `mpd_oled -o6 -r 24 ...`.
Specify the I2C bus number, if not 1,
with mpd_oled -B, e.g. `mpd_oled -o6 -B 0 ...`

For, SPI OLEDs (mpd_oled -o 1 or 7), you may need to specify your reset pin
GPIO number (mpd_oled -r, default 25), DC pin GPIO number (mpd_oled -D,
default 24) or CS value (mpd_oled -S, default 0).

If your display is upside down, you can rotate it 180 degrees with option '-R'.

Once the display is working, play some music and check the spectrum display
is working and is synchronised with the music. If there are no bars then the
audio copy may not have been configured correctly. If the bars seem jerky
or not synchronized with the music then reduce the values of -b and/or -f.

## Install the mpd_oled service

When you have chosen some suitable options, install and configure
an mpd_oled service file so that mpd_oled will run at boot.

Install a service file. This will not overwrite an existing mpd_oled
service file.
```
sudo mpd_oled_service_install
```

Edit the service file to include your chosen options. Rerun
this command any time to change the options. You *must* include a
valid -o parameter for your OLED. If the command appears to hang,
allow it some time to complete. If the included mpd_oled options are
valid then mpd_oled will start running on the display when the
command completes.

Either, run the command with no options, which will open an editor, then
add your options (from a successful mpd_oled test command) on the line
starting `ExecStart` and after `mpd_oled`.

```
sudo mpd_oled_service_edit     # edit mpd_oled options with editor
```

Or, append all your options (from a successful mpd_oled test command)
to the command and the service file will be updated to use these
optiond for mpd_oled, e.g. the following will cause the service to
run `mpd_oled -o 6 -b 10'
```
sudo mpd_oled_service_edit -o 6 -b 10
```

### Extra commands to control the service

Commands from the following list can be run to control the service
(they do not need to be run from the mpd_oled directory)
```
sudo systemctl enable mpd_oled    # start mpd_oled at boot
sudo systemctl disable mpd_oled   # don't start mpd_oled at boot
sudo systemctl start mpd_oled     # start mpd_oled now
sudo systemctl stop mpd_oled      # stop mpd_oled now
sudo systemctl status mpd_oled    # report the status of the service
```

If you wish to uninstall the mpd_oled service (just the service,
the command does not uninstall the mpd_oled or mpd_oled_cava binaries)
```
sudo mpd_oled_service_uninstall
```

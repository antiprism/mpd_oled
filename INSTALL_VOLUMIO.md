# Install instructions for Volumio

## Base system

Install [Volumio](https://volumio.org/). Ensure a command line prompt is
available for entering the commands below (e.g.
[use SSH](https://volumio.github.io/docs/User_Manual/SSH.html).)

## Build and install cava

mpd_oled uses Cava, a bar spectrum audio visualizer, to calculate the spectrum
   
   <https://github.com/karlstav/cava>

The commands to download, build and install Cava are as follows. Note that
the the command to install packages is different to the one given in the Cava
instructions. It installs extra packages needed for the build and does not
install the Pulseaudio development package.
```
sudo apt-get update
sudo apt-get install git-core autoconf make libtool libfftw3-dev libasound2-dev
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

### Configure copy of audio
The MPD audio output needs to be copied to a named pipe, where Cava can
read it and calculate the spectrum. This should be configured in /etc/mpd.conf,
but changes to this file will be overwritten by Volumio. Instead, edit the
mpd.conf template file
```
sudo nano /volumio/app/plugins/music_service/mpd/mpd.conf.tmpl
```
And add the following lines at the end
```
audio_output {
        type            "fifo"
        name            "mpd_oled_FIFO"
        path            "/tmp/mpd_oled_fifo"
        format          "44100:16:2"
}
```
After editing the file it is important to force Volumio to regenerate
mpd.conf and restart MPD. To do this, open the Web UI and go to
Settings > Playback Options then click on Save in the Audio Output section.

### Set time zone
If the mpd_oled clock does not display the local time then you may need
to set the system time zone. The following command will run a console
based application where you can specify your location
```
sudo dpkg-reconfigure tzdata
```

## Build and install mpd_oled

Install the packages needed to build the program
```
   sudo apt install build-essential git-core autoconf make libtool libi2c-dev i2c-tools lm-sensors libcurl4-openssl-dev libmpdclient-dev libjsoncpp-dev
```
Clone the source repository
```
git clone https://github.com/antiprism/mpd_oled
```
Change to the source directory and build the program
```
cd mpd_oled
PLAYER=VOLUMIO make
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



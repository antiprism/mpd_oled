# MPD, Volumio, RuneAudio and Moode OLED Spectrum Display for Raspberry Pi

The mpd_oled program displays an information screen including a music
frequency spectrum on an OLED screen connected to a Raspberry Pi (or similar)
running MPD, this includes Volumio, RuneAudio and Moode.
![OLED with mpd_oled](mpd_oled.jpg)

The code and installation instructions have not been widely tested. The
program supports a variety of I2C and SPI 128x64 OLED displays, but I only
have access to a 4-pin I2C SSH1106 OLED and a Raspberry Pi Zero W for
testing.

## Build and install

The instructions depend on the player

* [Build and install on Volumio](INSTALL_VOLUMIO.md)
* [Build and install on RuneAudio](INSTALL_RUNEAUDIO.md)
* [Build and install on Moode](INSTALL_MOODE.md)
* Build and install on Debian-based OS running MPD, follow the instructions
  for [Build and install on Volumio](INSTALL_VOLUMIO.md) but use
  `PLAYER=MPD make` when building mpd_oled


## Credits

C.A.V.A. is a bar spectrum audio visualizer: <https://github.com/karlstav/cava>

OLED interface based on ArduiPI_OLED: <https://github.com/hallard/ArduiPi_OLED>
(which is based on the Adafruit_SSD1306, Adafruit_GFX, and bcm2835 library
code).



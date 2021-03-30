# FAQ for mpd_oled

### How can I change the layout?

There are no configuration options to change the layout If you
wish to change the layout then you will need to change the code.

### How can I remove the spectrum analyser?

There are no configuration options to remove the spectrum If you
wish to change the layout then you will need to change the code.

### What code do I change to customise the display?

In main.cpp there are two simple functions that draw the layout

   draw_clock():         draws the stop screen
   draw_spect_display(): draws the play/pause screen

If you remove the spectrum analyser component then there is no need
to change any other code, just use a nonexistant file for the FIFO
and this should avoid any CPU usage, e.g.

   mpd_oled -c fifo,/tmp/dummy

### How do I make the spectrum analyser work?

The spectrum analyser needs a copy of the audio that is playing to
work, and it needs mpd_oled_cava (or cava) to be running.
Check that you followed *all* the instructions to set up the audio
copy on your system, and check that mpd_oled_cava (or cava) is running, e.g.
run ```ps ax | grep cava```.

If you have set up the audio copy using MPD then only sources played
through MPD will show a spectrum. Playing through Spotify or Airplay
may not show a spectrum. A spectrum can be enabled for all sources
by configuring ALSA to make the copy. See the thread on
[using mpd_oled with Spotify and Airplay](https://github.com/antiprism/mpd_oled/issues/4)

### Why is my screen flashing and corrupted?

You may be running two copies of mpd_oled. Check by running
```ps ax | grep mpd_oled```

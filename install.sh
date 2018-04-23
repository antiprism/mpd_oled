#!/bin/bash

systemctl stop mpd_oled
cp mpd_oled /usr/local/bin
cp mpd_oled.service /etc/systemd/system
systemctl daemon-reload
systemctl enable mpd_oled
systemctl start mpd_oled


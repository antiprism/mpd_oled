#!/bin/bash

service="mpd_oled"

systemctl is-active --quiet $service && systemctl stop $service
systemctl disable $service
rm /etc/systemd/system/$service.service
systemctl daemon-reload
systemctl reset-failed

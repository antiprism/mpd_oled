#!/bin/bash

service="mpd_oled"

if test ! -z "$*" ; then
   echo "setting service file to run: mpd_oled '$*'" 
   sed  -i "s/^\(ExecStart.*mpd_oled\)\(.*\)/\1 $*/" /etc/systemd/system/$service.service
else
   echo "editing the service file..."
   nano /etc/systemd/system/$service.service
fi

systemctl daemon-reload
systemctl restart $service

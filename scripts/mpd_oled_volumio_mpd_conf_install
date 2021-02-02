#!/bin/bash

# template file used to create /etc/mpd.conf
tmpl_file="/volumio/app/plugins/music_service/mpd/mpd.conf.tmpl"

# no need to continue if the file is already modified
if grep -q "/tmp/mpd_oled_fifo" "$tmpl_file"; then
  echo "mpd.conf template file aready includes a section for mpd_oled"
  exit 1
fi


#########################################
# ensure mpd and mpd_oled are not running

volumio stop

# record if MPD is initially running as service, and stop if running
if systemctl is-active --quiet mpd; then
  mpd_is_running="true"
  systemctl stop mpd
fi

# record if mpd_oled is initially running as service, and stop if running
if systemctl is-active --quiet mpd_oled; then
  mpd_oled_is_running="true"
  systemctl stop mpd_oled
fi

# Kill mpd_oled, mpd_oled_cava and cava, just to be sure, don't restart later
killall --quiet cava
killall --quiet mpd_oled_cava
killall --quiet mpd_oled

# Get rid of any stale FIFO
rm /tmp/mpd_oled_fifo


###############################
# modify template file

# audio_output section to add to the mpd.conf template file
mpd_conf_txt="
# add a FIFO to be read by a cava, which is run as a subprocess of mpd_oled
audio_output {
        type            "\""fifo"\""
        name            "\""mpd_oled_FIFO"\""
        path            "\""/tmp/mpd_oled_fifo"\""
        format          "\""44100:16:2"\""
}"

# append the text to the template file
echo "$mpd_conf_txt" >> "$tmpl_file"


##########################
# regenerate /etc/mpd.conf

# node script to call Volumio createMPDFile
# https://community.volumio.org/t/command-to-regenerate-mpd-conf/44573
mpd_conf_regen_js="const io = require('socket.io-client');
const socket = io.connect('http://localhost:3000');
const endPoint = { 'endpoint': 'music_service/mpd', 'method': 'createMPDFile', 'data': '' };
socket.emit('callMethod', endPoint);
setTimeout(() => process.exit(0), 1000);"

# create a temporary directory
tmp_dir=$(mktemp -d -t mpd_oled-XXXXXXXXXX)

# create a file with the script text
echo "$mpd_conf_regen_js" > "$tmp_dir/mpd_conf_regen.js"

# make the Volume node modules accessible to the script
ln -s /volumio/node_modules/ "$tmp_dir"

# run the script
node "$tmp_dir/mpd_conf_regen.js"

# tidy up
rm -rf "$tmp_dir"


##################
# restart services

# no need to start MPD, handled by Volumio

# start mpd_oled if initially running
if test "$mpd_oled_is_running" = "true"; then
  systemctl start mpd_oled
fi

exit 0

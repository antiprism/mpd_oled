/*
   Copyright (c) 2014-2018, Adrian Rossiter

   Antiprism - http://www.antiprism.com

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice shall be included
      in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
*/

/* \file status.cpp
   \brief get information about player status
*/

#include "player.h"

#include <stdlib.h>
#include <string>
#include <sys/stat.h>

using std::string;

std::vector<std::string> Player::name_strs = {"mpd", "moode", "volumio",
                                              "runeaudio","plexamp", "unknown"};

namespace {
bool file_exists(const std::string &name)
{
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}
} // namespace

void Player::init_detect()
{
  // Check for files associated with a player OS
  if (file_exists("/var/www/command/moode.php"))
    name = Player::Name::moode;
  else if (file_exists("/volumio"))
    name = Player::Name::volumio;
  else if (file_exists("/srv/http/command/rune_shutdown"))
    name = Player::Name::runeaudio;
  else if (file_exists("/home/claudio/plexamp/js/index.js"))
    name = Player::Name::plexamp;
  else if (system("which mpd > /dev/null 2>&1") == 0) // check for mpd
    name = Player::Name::mpd;
  else
    name = Player::Name::unknown;
}

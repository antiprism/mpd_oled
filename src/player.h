/*
   Copyright (c) 2018, Adrian Rossiter

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

#ifndef PLAYER_H
#define PLAYER_H

#include "timer.h"
#include "utils.h"

#include <algorithm>
#include <map>
#include <string.h>
#include <string>
#include <vector>

class Player {
public:
  // name values here, names strings define in cpp file
  enum Name { mpd, moode, volumio, runeaudio, plexamp, unknown };
  static std::vector<std::string> name_strs;

private:
  Name name = unknown;
  std::string version;

  static Name lookup_value(std::string nam)
  {
    auto it = std::find_if(name_strs.begin(), name_strs.end(),
                           [nam](std::string str) { return str == nam; });
    return (it != name_strs.end())
               ? static_cast<Name>(std::distance(name_strs.begin(), it))
               : Name::unknown;
  }

public:
  void init_detect(); // initialise with detected values
  void set_name(Name name_val) { name = name_val; }
  void set_name(std::string name_str) { name = lookup_value(name_str); }
  Name get_name() const { return name; }
  bool is(Name nam) const { return name == nam; }
  void set_version(std::string vers) { version = vers; }
  std::string get_version() const { return version; }
  std::string str() const
  {
    auto ret = name_strs[name];
    if (!version.empty())
      ret += " " + version;
    return ret;
  }

  static std::string all_names(std::string delim)
  {
    return join(name_strs.begin(), name_strs.end(), delim);
  }
};

#endif // PLAYER.H

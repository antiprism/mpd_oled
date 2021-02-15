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

#ifndef DISPLAY_INFO_H
#define DISPLAY_INFO_H

#include "status.h"
#include <vector>


struct spect_graph
{
  int gap;                                 // size of gap in pixels
  std::vector<unsigned char> heights;      // bar heights

  void init(int bars, int gap_sz) {gap = gap_sz;  heights.resize(bars, 0); }
};


struct display_info
{
  spect_graph spect;
  mpd_info status;
  Counter text_change;
  std::vector<double> scroll;
  int clock_format;
  int date_format;
  char pause_screen;
  connection_info conn;
  void conn_init() { conn.init(); }
  void update_from(const display_info &new_info);
};

inline void display_info::update_from(const display_info &new_info)
{
  bool changed = (status.get_title() != new_info.status.get_title() ||
                  status.get_origin() != new_info.status.get_origin() ||
                  status.get_state() != new_info.status.get_state());
  *this = new_info;
  if(changed)
    text_change.reset();
}

#endif // DISPLAY_INFO_H


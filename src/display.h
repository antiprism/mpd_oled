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

#ifndef DISPLAY_H
#define DISPLAY_H

#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

#include "display_info.h"

#include <string>
#include <vector>

// Draw spectrum graph
int draw_spectrum(ArduiPi_OLED &display, int x_start, int y_start, int width,
                  int height, const spect_graph &spect);

// Draw time HH:MM, according to what - 0: 24h leading 0, 1: 24h no leading 0
//                                      2: 24h leading 0, 3: 24h no leading 0
void draw_time(ArduiPi_OLED &display, int start_x, int start_y, int sz,
               int clock_format);

// Fraw date - DD-MM-YYYY
void draw_date(ArduiPi_OLED &display, int start_x, int start_y, int sz,
               int date_format);

// Draw a connection indicator, 12x8
void draw_connection(ArduiPi_OLED &display, int x_start, int y_start,
                     const connection_info &conn);

// Draw a slider
void draw_slider(ArduiPi_OLED &display, int x_start, int y_start, int width,
                 int height, float percent);

// Draw solid slider
void draw_solid_slider(ArduiPi_OLED &display, int x_start, int y_start,
                       int width, int height, float percent);

// Draw triangle slider
void draw_triangle_slider(ArduiPi_OLED &display, int x_start, int y_start,
                          int width, int height, float percent);

// Draw text
void draw_text(ArduiPi_OLED &display, int x_start, int y_start, int max_len,
               std::string str);

// Draw text and scroll in box
void draw_text_scroll(ArduiPi_OLED &display, int x_start, int y_start,
                      int max_len, std::string str, std::vector<double> scroll,
                      double secs = 0.0);

bool init_display(ArduiPi_OLED &display, int oled, unsigned char i2c_addr,
                  int i2c_bus, int reset_gpio, int spi_dc_gpio, int spi_cs,
                  bool rotate180 = false);

#endif // DISPLAY_H

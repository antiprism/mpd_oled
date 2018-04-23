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

#include "display.h"

#include <vector>
#include <string>

using std::vector;
using std::string;

int draw_spectrum(ArduiPi_OLED &display, int x_start, int y_start, int width,
    int height, const spect_graph &spect)
{
  const int num_bars = spect.heights.size();
  const int gap = spect.gap;

  int total_bar_pixes = width-(num_bars-1)*gap;
  int bar_width = total_bar_pixes / num_bars;
  int bar_height_max = height - 1;
  int graph_width = num_bars*bar_width + (num_bars-1)*gap;

  if(bar_width < 1 || bar_height_max < 1)  // bars too small to draw
    return -1;

  // Draw spectrum graph axes
  display.drawFastHLine(x_start, height - 1 - y_start, graph_width, WHITE);
  for (int i=0; i<num_bars; i++) {
    // map vals range to graph ht
    int val = bar_height_max * spect.heights[i] / 255.0 + 0.5;
    int x = x_start + i*(bar_width+gap);
    int y = y_start+2;
    if(val)
       display.fillRect(x, y_start + height - val - 2, bar_width, val, WHITE);
  }
  return 0;
}

// Draw time, according to what: 0 - time HH:MM, 1 - date DD-MM-YYYY
void draw_time(ArduiPi_OLED &display, int start_x, int start_y, int sz,
    int what)
{
  display.setTextColor(WHITE);
  
  time_t t = time(0);
  struct tm *now = localtime(&t);
  const size_t STR_SZ = 32;
  char str[STR_SZ];
  if (what == 0)
    strftime(str, STR_SZ, "%H:%M", now);
  else if (what == 1)
    strftime(str, STR_SZ, "%d-%m-%Y", now);
  else
    str[0] = '\0';
  
  display.setCursor(start_x, start_y);
  display.setTextSize(sz);
  display.print(str);
}


// Draw a connection indicator, 12x8
void draw_connection(ArduiPi_OLED &display, int x_start, int y_start, int val)
{
  const int char_ht = 8;
  if (val > 100) {                  // wifi signal strength (>0)
    int signal = val - 100;
    for(int i=0; i<4; i++) {
      int ht = 2*i + 1;
      int x_off = 3*i + 1;
      if (signal > 20*i)
        display.fillRect(x_start+x_off, y_start+char_ht-(1+ht), 2, ht, WHITE);
      else
        break;
    }
  }
  else if (val >= 0 && val < 100) {             // ethernet
    int w = 10;
    display.drawPixel(x_start+3, y_start, WHITE);
    display.drawFastHLine(x_start+1+1, y_start+1, w-1, WHITE);
    display.drawFastHLine(x_start+1, y_start+2, w, WHITE);
    display.drawFastHLine(x_start+1, y_start+4, w, WHITE);
    display.drawFastHLine(x_start+1, y_start+5, w-1, WHITE);
    display.drawPixel(x_start+1+w-3, y_start+6, WHITE);
  }
}

// Draw a slider
void draw_slider(ArduiPi_OLED &display, int x_start, int y_start, int width,
    int height, float percent)
{
  const int in_width = (width - 2) * percent/100.0 + 0.5;
  display.drawRect(x_start, y_start, width, height, WHITE);
  display.fillRect(x_start+1, y_start+1, in_width, height-2, WHITE);
}

// Draw solid slider
void draw_solid_slider(ArduiPi_OLED &display, int x_start, int y_start,
    int width, int height, float percent)
{
  const int bar_width = width * percent/100.0 + 0.5;
  display.fillRect(x_start, y_start, bar_width, height, WHITE);
}

// Draw triangle slider
void draw_triangle_slider(ArduiPi_OLED &display, int x_start, int y_start,
    int width, int height, float percent)
{
  const float frac = percent/100;
  display.fillTriangle(
      x_start, y_start + height-1,
      x_start + (width-1)*frac, y_start + height-1,
      x_start + (width-1)*frac, y_start + (height-1)*(1-frac), WHITE);
}

// Draw text
void draw_text(ArduiPi_OLED &display, int x_start, int y_start, int max_len,
    string str)
{
  if ((int)str.size() > max_len)
    str.resize(max_len);

  display.setTextColor(WHITE);
  display.setCursor(x_start, y_start);
  display.setTextSize(1);
  display.print(str.c_str());
}


bool init_display(ArduiPi_OLED &display, int oled, unsigned char i2c_addr,
    int reset_gpio)
{
// SPI
  if (display.oled_is_spi_proto(oled)) {
    // SPI change parameters to fit to your LCD
    if ( !display.init(OLED_SPI_DC, reset_gpio, OLED_SPI_CS, oled) )
      return false;
  }
  else {
    // I2C change parameters to fit to your LCD
    if ( !display.init(reset_gpio, oled, i2c_addr) )
      return false;
  }

  display.begin();

  // init done
  display.clearDisplay();       // clears the screen  buffer
  display.display();   		// display it (clear display)

  return true;
}



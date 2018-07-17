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
#include <algorithm>

using std::vector;
using std::string;

void print(ArduiPi_OLED &display, const char *str)
{
  int sz = strlen(str);
  for(int i=0; i<sz; i++)
    display.write((uint8_t)str[i]);
}

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

// Draw time, according to clock_format: 0-3
void draw_time(ArduiPi_OLED &display, int start_x, int start_y, int sz,
    int clock_format)
{
  display.setTextColor(WHITE);

  time_t t = time(0);
  struct tm *now = localtime(&t);
  const size_t STR_SZ = 32;
  char str[STR_SZ];
  const char *fmts[] = { "%H:%M", "%k:%M", "%I:%M", "%l:%M" };
  if (clock_format >= 0 || clock_format < (int)sizeof(fmts))
    strftime(str, STR_SZ, fmts[clock_format], now);
  else
    str[0] = '\0';

  display.setCursor(start_x, start_y);
  display.setTextSize(sz);
  print(display, str);
  int W = 6; // width of a character box
  int N = 5; // number of character
  if (now->tm_hour >= 12)
     display.fillRect(start_x+W*N*sz, start_y, sz, sz, WHITE);
}


// Draw date - DD-MM-YYYY
void draw_date(ArduiPi_OLED &display, int start_x, int start_y, int sz)
{
  display.setTextColor(WHITE);

  time_t t = time(0);
  struct tm *now = localtime(&t);
  const size_t STR_SZ = 32;
  char str[STR_SZ];
  strftime(str, STR_SZ, "%d-%m-%Y", now);

  display.setCursor(start_x, start_y);
  display.setTextSize(sz);
  print(display, str);
}


// Draw a connection indicator, 12x8
void draw_connection(ArduiPi_OLED &display, int x_start, int y_start,
  const connection_info &conn)
{
  const int char_ht = 8;
  if (conn.get_type() == connection_info::TYPE_WIFI) {
    for(int i=0; i<4; i++) {
      int ht = 2*i + 1;
      int x_off = 3*i + 1;
      if (conn.get_link() > 20*i)
        display.fillRect(x_start+x_off, y_start+char_ht-(1+ht), 2, ht, WHITE);
      else
        break;
    }
  }
  else if (conn.get_type() == connection_info::TYPE_ETH) {
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
  print(display, str.c_str());
}

// Draw text
void draw_text_scroll(ArduiPi_OLED &display, int x_start, int y_start,
    int max_len, string str, vector<double> scroll, double secs)
{
  if ((int)str.size() <= max_len) {
    draw_text(display, x_start, y_start, max_len, str);
    return;
  }
  const double pixels_per_sec = scroll[0];
  const double scroll_after_secs = scroll[1];

  int size = 1;
  int W = 6*size;
  str += "     ";
  double elapsed = secs - scroll_after_secs;
  int pix_shift = (elapsed<0) ? 0.0
                  : int(elapsed*pixels_per_sec + 0.5) % (str.size()*W);
  int pix_offset = pix_shift % W;
  int char_pix_offset = (W-pix_offset)%W;
  int char_shift = pix_shift / W + (char_pix_offset>0);
  std::rotate(str.begin(), str.begin()+char_shift, str.end());
  //fprintf(stderr, "%c : %4d : %4d : %4d : %4d\n", str[0], pix_shift, pix_offset, char_pix_offset, char_shift);

  display.setTextColor(WHITE);
  display.setTextSize(size);
  display.setCursor(x_start, y_start);
  // Draw first partial character
  if(char_pix_offset>0)
      display.drawCharPart(x_start, y_start, W - char_pix_offset, W,
          str[str.size()-1], WHITE, BLACK, 1);
  str.resize(max_len+1);
  display.setCursor(x_start+char_pix_offset, y_start);
  // Draw intermediate characters
  print(display, str.substr(0, max_len-1).c_str());
  // Draw last partial character
  display.drawCharPart(x_start + (max_len-1)*W + char_pix_offset, y_start,
      0, pix_offset ? pix_offset : W, str[max_len-1], WHITE, BLACK, 1);
}

static void set_rotation(ArduiPi_OLED &display, bool upside_down)
{
  if(upside_down) {
    display.sendCommand(0xA0);
    display.sendCommand(0xC0);
  }
  else {
    display.sendCommand(0xA1);
    display.sendCommand(0xC8);
  }
}

bool init_display(ArduiPi_OLED &display, int oled, unsigned char i2c_addr,
    int reset_gpio, bool rotate180)
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
  
  set_rotation(display, rotate180);
  display.setTextWrap(false);

  // init done
  display.clearDisplay();       // clears the screen  buffer
  display.display();   		// display it (clear display)

  return true;
}



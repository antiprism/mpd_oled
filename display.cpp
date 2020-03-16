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
    // int y = y_start+2;
    if(val)
       display.fillRect(x, y_start + height - val - 2, bar_width, val, WHITE);
  }
  return 0;
}

// Draw mpd state
void draw_mpd_state(ArduiPi_OLED &display, const display_info &disp_info, int x_start, int y_start,
    int width, int height, enum mpd_state state)
{
  string state_text = "";
  if (state == MPD_STATE_STOP)
    state_text = "STOP";
  else if (state == MPD_STATE_PLAY)
   state_text = "PLAY";
  else if (state == MPD_STATE_PAUSE)
    state_text = "PAUSED";
  else
   state_text = "STATE UNKNOWN";

  vector<double> scroll_origin(disp_info.scroll.begin()+2,
                               disp_info.scroll.begin()+4);
  draw_text_scroll(display, 1, 3, 5, state_text, scroll_origin, disp_info.text_change.secs(), 2);
  display.drawFastHLine(x_start, height-1-y_start, width, WHITE);
}

// Draw moOde logo
void draw_moode_logo(ArduiPi_OLED &display)
{
  // m
display.fillRect(1, 25, 3, 24, WHITE);
display.fillRect(4, 26, 2, 23, WHITE);
display.drawLine(6, 37, 6, 47, WHITE);
display.drawLine(7, 34, 7, 43, WHITE);
display.drawLine(8, 32, 8, 38, WHITE);
display.drawLine(10, 29, 12, 29, WHITE);
display.drawLine(9, 30, 13, 30, WHITE);
display.fillRect(9, 31, 6, 5, WHITE);
display.fillRect(10, 36, 5, 11, WHITE);
display.drawLine(11, 47, 15, 47, WHITE);
display.drawLine(12, 48, 14, 48, WHITE);
display.drawLine(15, 40, 15, 46, WHITE);
display.drawLine(16, 37, 16, 44, WHITE);
display.drawLine(17, 34, 17, 40, WHITE);
display.drawLine(18, 32, 18, 37, WHITE);
display.drawLine(19, 29, 19, 35, WHITE);
display.drawLine(21, 27, 22, 27, WHITE);
display.drawLine(25, 29, 25, 34, WHITE);
display.drawLine(26, 30, 26, 31, WHITE);
display.fillRect(20, 28, 5, 16, WHITE);
display.fillRect(21, 44, 5, 3, WHITE);
display.fillRect(22, 47, 8, 2, WHITE);
display.drawLine(23, 49, 28, 49, WHITE);
display.drawPixel(26, 46, WHITE);
display.drawPixel(28, 46, WHITE);
display.drawPixel(29, 46, WHITE);
display.drawPixel(29, 45, WHITE);
display.drawLine(30, 43, 30, 47, WHITE);

// o
display.fillRect(32, 32, 5, 10, WHITE);
display.fillRect(34, 26, 4, 5, WHITE);
display.fillRect(45, 30, 4, 14, WHITE);
display.fillRect(37, 39, 8, 3, WHITE);
display.fillRect(32, 42, 4, 3, WHITE);
display.fillRect(38, 47, 7, 3, WHITE);
display.fillRect(36, 23, 8, 3, WHITE);
display.fillRect(45, 27, 3, 3, WHITE);
display.fillRect(44, 44, 3, 3, WHITE);
display.drawLine(31, 39, 31, 45, WHITE);
display.drawLine(33, 31, 36, 31, WHITE);
display.drawLine(37, 22, 41, 22, WHITE);
display.drawLine(44, 24, 44, 27, WHITE);
display.drawLine(43, 26, 46, 26, WHITE);
display.drawLine(36, 43, 36, 48, WHITE);
display.drawLine(33, 45, 35, 45, WHITE);
display.drawLine(34, 46, 38, 46, WHITE);
display.drawLine(41, 46, 43, 46, WHITE);
display.drawLine(35, 47, 46, 47, WHITE);
display.drawLine(37, 48, 45, 48, WHITE);
display.drawLine(49, 36, 49, 39, WHITE);
display.drawLine(50, 35, 50, 38, WHITE);
display.drawLine(51, 33, 51, 36, WHITE);
display.drawLine(52, 31, 52, 35, WHITE);
display.drawPixel(35, 24, WHITE);
display.drawPixel(35, 25, WHITE);
display.drawPixel(33, 29, WHITE);
display.drawPixel(33, 30, WHITE);
display.drawPixel(38, 26, WHITE);
display.drawPixel(39, 26, WHITE);
display.drawPixel(38, 27, WHITE);
display.drawPixel(45, 25, WHITE);
display.drawPixel(37, 37, WHITE);
display.drawPixel(37, 38, WHITE);
display.drawPixel(38, 38, WHITE);
display.drawPixel(44, 38, WHITE);
display.drawPixel(43, 45, WHITE);
display.drawPixel(47, 44, WHITE);
display.drawPixel(47, 45, WHITE);

// O
display.drawLine(53, 26, 53, 37, WHITE);
display.drawLine(54, 23, 54, 39, WHITE);
display.drawLine(55, 20, 55, 42, WHITE);
display.drawLine(56, 18, 56, 43, WHITE);
display.drawLine(57, 17, 57, 44, WHITE);
display.drawLine(58, 15, 58, 45, WHITE);
display.fillRect(59, 15, 5, 10, WHITE);
display.drawLine(62, 11, 68, 11, WHITE);
display.drawLine(61, 12, 73, 12, WHITE);
display.drawLine(60, 13, 75, 13, WHITE);
display.drawLine(59, 14, 66, 14, WHITE);
display.drawLine(71, 14, 76, 14, WHITE);
display.drawLine(73, 15, 77, 15, WHITE);
display.drawLine(74, 16, 78, 16, WHITE);
display.fillRect(75, 17, 5, 2, WHITE);
display.fillRect(76, 19, 5, 2, WHITE);
display.drawLine(77, 21, 77, 23, WHITE);
display.drawLine(78, 21, 78, 44, WHITE);
display.drawLine(79, 21, 79, 43, WHITE);
display.drawLine(80, 21, 80, 42, WHITE);
display.drawLine(81, 22, 81, 40, WHITE);
display.drawLine(82, 26, 82, 37, WHITE);
display.drawLine(59, 46, 76, 46, WHITE);
display.drawLine(60, 47, 75,47, WHITE);
display.drawLine(62, 48, 73, 48, WHITE);
display.drawLine(66, 49, 69, 49, WHITE);
display.drawLine(59, 39, 59, 45, WHITE);
display.drawLine(77, 38, 77, 45, WHITE);
display.drawLine(60, 45, 64, 45, WHITE);
display.drawLine(72, 45, 76, 45, WHITE);
display.drawLine(76, 41, 76, 44, WHITE);
display.drawLine(60, 42, 60, 44, WHITE);
display.drawLine(61, 44, 62, 44, WHITE);
display.drawLine(74, 44, 75, 44, WHITE);
display.drawPixel(61, 43, WHITE);
display.drawPixel(62, 44, WHITE);
display.drawPixel(75, 43, WHITE);
display.drawLine(60, 25, 64, 25, WHITE);
display.drawLine(61, 26, 65, 26, WHITE);
display.drawLine(61, 27, 66, 27, WHITE);
display.drawLine(62, 28, 68, 28, WHITE);
display.drawLine(64, 29, 74, 29, WHITE);
display.drawLine(66, 30, 73, 30, WHITE);
display.drawLine(73, 28, 74, 28, WHITE);
display.drawLine(64, 15, 65, 15, WHITE);
display.drawPixel(64, 16, WHITE);
display.drawPixel(64, 24, WHITE);
display.drawPixel(74, 27, WHITE);

// d
display.fillRect(98, 7, 5, 38, WHITE);
display.fillRect(100, 45, 8, 3, WHITE);
display.fillRect(87, 31, 3, 17, WHITE);
display.drawLine(102, 48, 105,48, WHITE);
display.drawPixel(99, 45, WHITE);
display.drawPixel(103, 44, WHITE);
display.drawPixel(107, 44, WHITE);
display.drawLine(103, 7, 103, 15, WHITE);
display.drawLine(97, 36, 97, 46, WHITE);
display.drawLine(96, 40, 96, 47, WHITE);
display.drawLine(95, 45, 95, 49, WHITE);
display.drawLine(90, 49, 94, 49, WHITE);
display.drawLine(88, 48, 94, 48, WHITE);
display.drawLine(85, 39, 85, 41, WHITE);
display.drawLine(86, 34, 86, 45, WHITE);
display.drawLine(90, 45, 90, 47, WHITE);
display.drawPixel(94, 47, WHITE);
display.fillRect(88, 28, 4, 6, WHITE);
display.drawLine(87, 31, 87, 33, WHITE);
display.drawLine(90, 34, 90, 39, WHITE);
display.drawLine(91, 25, 96, 25, WHITE);
display.drawLine(90, 26, 97, 26, WHITE);
display.drawLine(89, 27, 97, 27, WHITE);
display.drawLine(92, 28, 94, 28, WHITE);
display.drawLine(96, 28, 97, 28, WHITE);
display.drawLine(92, 29, 92, 30, WHITE);
display.drawLine(97, 7, 97, 8, WHITE);
display.drawLine(108, 42, 108, 46, WHITE);

// e
display.drawLine(109, 34, 109, 44, WHITE);
display.drawLine(110, 31, 110, 46, WHITE);
display.drawLine(111, 29, 111, 47, WHITE);
display.drawLine(112, 27, 112, 48, WHITE);
display.drawLine(113, 26, 113, 49, WHITE);
display.drawLine(114, 26, 114, 35, WHITE);
display.fillRect(115, 25, 5, 4, WHITE);
display.drawLine(120, 26, 120, 37, WHITE);
display.drawLine(121, 27, 121, 36, WHITE);
display.drawLine(122, 28, 122, 33, WHITE);
display.fillRect(114, 38, 3, 3, WHITE);
display.drawLine(117, 37, 117, 40, WHITE);
display.drawLine(118, 36, 118, 39, WHITE);
display.drawLine(119, 34, 119, 38, WHITE);
display.drawLine(115, 29, 115, 31, WHITE);
display.fillRect(114, 47, 8, 3, WHITE);
display.drawLine(114, 44, 114, 46, WHITE);
display.drawLine(122, 46, 122, 48, WHITE);
display.drawLine(123, 45, 123, 47, WHITE);
display.drawLine(124, 43, 124, 46, WHITE);
display.drawLine(125, 39, 125, 45, WHITE);
display.drawLine(126, 38, 126, 43, WHITE);
display.drawPixel(116, 29, WHITE);
display.drawPixel(115, 46, WHITE);
display.drawLine(117, 50, 119, 50, WHITE);
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
  if (clock_format > 1 && now->tm_hour >= 12)
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
    string str, int sz)
{
  if ((int)str.size() > max_len)
    str.resize(max_len);

  display.setTextColor(WHITE);
  display.setCursor(x_start, y_start);
  display.setTextSize(sz);
  print(display, str.c_str());
}

// Draw text
void draw_text_scroll(ArduiPi_OLED &display, int x_start, int y_start,
    int max_len, string str, vector<double> scroll, double secs, int sz)
{
  if ((int)str.size() <= max_len) {
    draw_text(display, x_start, y_start, max_len, str);
    return;
  }
  const double pixels_per_sec = scroll[0];
  const double scroll_after_secs = scroll[1];

  int size = sz;
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
    int i2c_bus, int reset_gpio, int spi_dc_gpio, int spi_cs, bool rotate180)
{
  if (display.oled_is_spi_proto(oled)) {
    // SPI change parameters to fit to your LCD
    if ( !display.init(spi_dc_gpio, reset_gpio, spi_cs, oled) )
      return false;
    bcm2835_spi_set_speed_hz(1e6); // ~1MHz
  }
  else {
    // I2C change parameters to fit to your LCD
    if ( !display.init(reset_gpio, oled, i2c_addr, i2c_bus) )
      return false;
  }

  display.begin();

  set_rotation(display, rotate180);
  display.setTextWrap(false);

  // init done
  display.clearDisplay();       // clears the screen  buffer
  display.display();            // display it (clear display)

  return true;
}



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

#include <algorithm>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace {
void set_clip_on(U8G2 &u8g2, int x_start, int y_start, int x_len, int y_len)
{
  if(x_len >= 0 && y_len >= 0)
    u8g2.setClipWindow(x_start, y_start, x_start + x_len, y_start + y_len);
}

void set_clip_off(U8G2 &u8g2, int x_len, int y_len)
{
  if(x_len >= 0 && y_len >= 0)
    u8g2.setMaxClipWindow();
}

void set_font(U8G2 &u8g2, const uint8_t *font)
{
  if(font)
      u8g2.setFont(font);
}
} // namespace

int draw_spectrum(U8G2 &u8g2, int x_start, int y_start, int width,
                  int height, const spect_graph &spect)
{
  const int num_bars = spect.heights.size();
  const int gap = spect.gap;

  int total_bar_pixes = width - (num_bars - 1) * gap;
  int bar_width = total_bar_pixes / num_bars;
  int bar_height_max = height - 2;
  int graph_width = num_bars * bar_width + (num_bars - 1) * gap;

  if (bar_width < 1 || bar_height_max < 1) // bars too small to draw
    return -1;

  // Draw spectrum graph axes
  u8g2.drawHLine(x_start, height - 1 - y_start, graph_width);
  for (int i = 0; i < num_bars; i++) {
    // map vals range to graph ht
    int val = bar_height_max * spect.heights[i] / 255.0 + 0.5;
    int x = x_start + i * (bar_width + gap);
    // int y = y_start+2;
    int y_bar_start = std::max(y_start + height - val - 2, 0);
    if (val)
      u8g2.drawBox(x, y_bar_start, bar_width, val);
  }
  return 0;
}

// Draw time, according to clock_format: 0-3
void draw_time(U8G2 &u8g2, int x_start, int y_start, int clock_format,
    const uint8_t *font)
{
  time_t t = time(0);
  struct tm *now = localtime(&t);
  const size_t STR_SZ = 32;
  char str[STR_SZ];
  const char *fmts[] = { "%H:%M", "%k:%M", "%I:%M", "%l:%M" };
  if (clock_format >= 0 || clock_format < (int)sizeof(fmts))
    strftime(str, STR_SZ, fmts[clock_format], now);
  else
    str[0] = '\0';

  set_font(u8g2, font);
  auto len = u8g2.drawUTF8(x_start, y_start, str);
  if (clock_format > 1 && now->tm_hour >= 12)
     u8g2.drawUTF8(x_start+len, y_start, ".");
}


// Draw date - DD-MM-YYYY
void draw_date(U8G2 &u8g2, int x_start, int y_start, int date_format,
    const uint8_t *font)
{
  time_t t = time(0);
  struct tm *now = localtime(&t);
  const size_t STR_SZ = 32;
  char str[STR_SZ];
  if (date_format == 1) // MM-DD-YYYY
    strftime(str, STR_SZ, "%m-%d-%Y", now);
  else // DD-MM-YYYY
    strftime(str, STR_SZ, "%d-%m-%Y", now);

  set_font(u8g2, font);
  u8g2.drawUTF8(x_start, y_start, str);
}

// Draw a connection indicator, 12x8
void draw_connection(U8G2 &u8g2, int x_start, int y_start,
                     const connection_info &conn)
{
  const int height = 8;
  if (conn.get_type() == connection_info::TYPE_WIFI) {
    for (int i = 0; i < 4; i++) {
      int bar_ht = 2 * i + 1;
      int x_off = 3 * i + 1;
      if (conn.get_link() > 20 * i)
        u8g2.drawBox(x_start+x_off, y_start + height - (bar_ht), 2, bar_ht);
      else
        break;
    }
  }
  else if (conn.get_type() == connection_info::TYPE_ETH) {
    int w = 10;
    u8g2.drawPixel(x_start+3, y_start);
    u8g2.drawHLine(x_start+1+1, y_start+1, w-1);
    u8g2.drawHLine(x_start+1, y_start+2, w);
    u8g2.drawHLine(x_start+1, y_start+4, w);
    u8g2.drawHLine(x_start+1, y_start+5, w-1);
    u8g2.drawPixel(x_start+1+w-3, y_start+6);
  }
}

// Draw a slider
void draw_slider(U8G2 &u8g2, int x_start, int y_start, int width,
                 int height, float percent)
{
  const int in_width = (width - 2) * percent / 100.0 + 0.5;
  u8g2.drawFrame(x_start, y_start, width, height);
  u8g2.drawBox(x_start + 1, y_start + 1, in_width, height - 2);
}

// Draw solid slider
void draw_solid_slider(U8G2 &u8g2, int x_start, int y_start,
                       int width, int height, float percent)
{
  const int bar_width = width * percent / 100.0 + 0.5;
  u8g2.drawBox(x_start, y_start, bar_width, height);
}

// Draw triangle slider
void draw_triangle_slider(U8G2 &u8g2, int x_start, int y_start,
                          int width, int height, float percent)
{
  // put percent in range 0 - 100
  percent = (percent < 0) ? 0 : (percent > 100) ? 100 : percent;
  const float frac = percent / 100;
  //u8g2.drawTriangle(x_start, y_start + height - 1,
  //                     x_start + (width - 1) * frac, y_start + height - 1,
  //                     x_start + (width - 1) * frac,
  //                     y_start + (height - 1) * (1 - frac));
  u8g2.drawTriangle(x_start, y_start + height, x_start + (width)*frac,
                    y_start + height, x_start + (width)*frac,
                    y_start + (height) * (1 - frac));
}

// Draw text
void draw_text(U8G2 &u8g2, int x_start, int y_start, string str,
               const uint8_t *font, int x_len, int y_len)
{
  set_font(u8g2, font);
  set_clip_on(u8g2, x_start, y_start, x_len, y_len);
  u8g2.drawUTF8(x_start, y_start, str.c_str());
  set_clip_off(u8g2, x_len, y_len);
}


// Draw text
void draw_text_scroll(U8G2 &u8g2, int x_start, int y_start, string str,
                      vector<double> scroll, double secs, const uint8_t *font,
                      int x_len, int y_len)
{

  set_font(u8g2, font);
  int width_str = u8g2.getUTF8Width(str.c_str());
  if (x_len >= 0 && width_str < x_len)  // no need to scroll
    draw_text(u8g2, x_start, y_start, str);
  else {                            // need to scroll
    const double pixels_per_sec = scroll[0];
    const double scroll_after_secs = scroll[1];

    string spaces = "     ";
    int width_spaces = u8g2.getUTF8Width(spaces.c_str());

    str += spaces + str;

    double elapsed = secs - scroll_after_secs;
    int pix_shift = (elapsed < 0) ? 0.0
                                  : int(elapsed * pixels_per_sec + 0.5) %
                                        (width_str + width_spaces);

    int clip_x_start = std::max(x_start, 0);
    int clip_y_start = std::max(y_start, 0);
    int clip_x_len = std::min(x_start - clip_x_start + x_len,
                              u8g2.getDisplayWidth() - clip_x_start - 1);
    int clip_y_len = std::min(y_start - clip_y_start + y_len,
                              u8g2.getDisplayHeight() - clip_y_start - 1);
    set_clip_on(u8g2, clip_x_start, clip_y_start, clip_x_len, clip_y_len);
    // fprintf(stderr, "clip (%d, %d, %d, %d)\n", clip_x_start, clip_y_start,
    // clip_x_len, clip_y_len);
    u8g2.drawUTF8(x_start - pix_shift, y_start, str.c_str());
    set_clip_off(u8g2, clip_x_len, clip_y_len);
  }
}

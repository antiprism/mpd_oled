/*
Universal 8bit Graphics Library (http://code.google.com/p/u8g2/)

Copyright (c) 2016, olikraus@gmail.com
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Original file: u8x8_d_sdl.cpp
Modified by:   Adrian Rossiter <adrian@antiprism.com>
Date:          10/04/2021
Changes:       Allow window size to be set. Remove unneeded code.
*/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "u8x8_d_sdl.h"

#include <libu8g2arm/u8g2.h>
#include <assert.h>

#ifdef ENABLE_SDL
#include <SDL.h>

#define W(x, w) (((x) * (w)) / 100)

SDL_Window *u8g_sdl_window;
SDL_Surface *u8g_sdl_screen;
int u8g_sdl_multiple = 3;
uint32_t u8g_sdl_color[256];
int u8g_sdl_height;
int u8g_sdl_width;

static void u8g_sdl_set_pixel(int x, int y, int idx)
{
  uint32_t *ptr;
  uint32_t offset;
  int i, j;

  if (y >= u8g_sdl_height)
    return;
  if (y < 0)
    return;
  if (x >= u8g_sdl_width)
    return;
  if (x < 0)
    return;

  for (i = 0; i < u8g_sdl_multiple; i++)
    for (j = 0; j < u8g_sdl_multiple; j++) {
      offset =
          (((y * u8g_sdl_multiple) + i) * (u8g_sdl_width * u8g_sdl_multiple) +
           ((x * u8g_sdl_multiple) + j)) *
          u8g_sdl_screen->format->BytesPerPixel;

      assert( offset < (Uint32)(u8g_sdl_width * u8g_sdl_multiple *
      u8g_sdl_height * u8g_sdl_multiple *
      u8g_sdl_screen->format->BytesPerPixel) );

      ptr = u8g_sdl_screen->pixels + offset;
      *ptr = u8g_sdl_color[idx];
    }
}

static void u8g_sdl_set_8pixel(int x, int y, uint8_t pixel)
{
  int cnt = 8;
  int bg = 0;
  if ((x / 8 + y / 8) & 1)
    bg = 4;
  while (cnt > 0) {
    if ((pixel & 1) == 0) {
      u8g_sdl_set_pixel(x, y, bg);
    }
    else {
      u8g_sdl_set_pixel(x, y, 3);
    }
    pixel >>= 1;
    y++;
    cnt--;
  }
}

static void u8g_sdl_set_multiple_8pixel(int x, int y, int cnt, uint8_t *pixel)
{
  uint8_t b;
  while (cnt > 0) {
    b = *pixel;
    u8g_sdl_set_8pixel(x, y, b);
    x++;
    pixel++;
    cnt--;
  }
}

static void u8g_sdl_init(int width, int height)
{
  u8g_sdl_height = height;
  u8g_sdl_width = width;

  //if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) != 0)
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    printf("Unable to initialize SDL:  %s\n", SDL_GetError());
    exit(1);
  }

  u8g_sdl_window = SDL_CreateWindow(
      "U8g2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      u8g_sdl_width * u8g_sdl_multiple, u8g_sdl_height * u8g_sdl_multiple, 0);

  if (u8g_sdl_window == NULL) {
    printf("Couldn't create window: %s\n", SDL_GetError());
    exit(1);
  }

  u8g_sdl_screen = SDL_GetWindowSurface(u8g_sdl_window);

  if (u8g_sdl_screen == NULL) {
    printf("Couldn't create screen: %s\n", SDL_GetError());
    exit(1);
  }

  printf("%d bits-per-pixel mode\n", u8g_sdl_screen->format->BitsPerPixel);
  printf("%d bytes-per-pixel mode\n", u8g_sdl_screen->format->BytesPerPixel);

  u8g_sdl_color[0] = SDL_MapRGB(u8g_sdl_screen->format, 0, 0, 0);
  u8g_sdl_color[1] =
      SDL_MapRGB(u8g_sdl_screen->format, W(100, 50), W(255, 50), 0);
  u8g_sdl_color[2] =
      SDL_MapRGB(u8g_sdl_screen->format, W(100, 80), W(255, 80), 0);
  u8g_sdl_color[3] = SDL_MapRGB(u8g_sdl_screen->format, 100, 255, 0);
  u8g_sdl_color[4] = SDL_MapRGB(u8g_sdl_screen->format, 30, 30, 30);

  /*
  u8g_sdl_set_pixel(0,0);
  u8g_sdl_set_pixel(1,1);
  u8g_sdl_set_pixel(2,2);
  */

  /* update all */
  SDL_UpdateWindowSurface(u8g_sdl_window);

  atexit(SDL_Quit);

  return;
}

typedef struct setup_info {
  u8x8_display_info_t u8x8_sdl_info;
  uint8_t *buf;
  int width;
  int height;
} setup_info;

void setup_info_init(setup_info *setup, int w, int h)
{
  setup->width = 8 * ((w + 8 - 1) / 8);
  setup->height = 8 * ((h + 8 - 1) / 8);
  size_t num_bytes = setup->width * setup->height / 8;
  setup->buf = (uint8_t *)malloc(num_bytes);
  /*
  fprintf(stderr, "num_bytes = %lu\n", num_bytes);
  fprintf(stderr, "width = %d, height = %d\n", setup->width, setup->height);
  exit(0);
  */
  setup->u8x8_sdl_info = (u8x8_display_info_t) {
    /* chip_enable_level = */ 0,
    /* chip_disable_level = */ 1,

    /* post_chip_enable_wait_ns = */ 0,
    /* pre_chip_disable_wait_ns = */ 0,
    /* reset_pulse_width_ms = */ 0,
    /* post_reset_wait_ms = */ 0,
    /* sda_setup_time_ns = */ 0,
    /* sck_pulse_width_ns = */ 0,
    /* sck_clock_hz = */ 4000000UL, /* since Arduino 1.6.0, the SPI bus speed in
                                       Hz. Should be
                                       1000000000/sck_pulse_width_ns */
    /* spi_mode = */ 1,
    /* i2c_bus_clock_100kHz = */ 0,
    /* data_setup_time_ns = */ 0,
    /* write_pulse_width_ns = */ 0,
    /* tile_width = */ setup->width / 8,
    /* tile_hight = */ setup->height / 8,
    /* default_x_offset = */ 0,
    /* flipmode_x_offset = */ 0,
    /* pixel_width = */ setup->width,
    /* pixel_height = */ setup->height
  };
}

static uint8_t u8x8_d_sdl_gpio(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                               void * arg_ptr)
{
  (void) arg_int;
  (void) arg_ptr;
  static int debounce_cnt = 0;
  static int curr_msg = 0;

  if (curr_msg > 0) {
    if (msg == curr_msg) {
      u8x8_SetGPIOResult(u8x8, 0);
      if (debounce_cnt == 0)
        curr_msg = 0;
      else
        debounce_cnt--;
      return 1;
    }
  }
  else
     u8x8_SetGPIOResult(u8x8, 1);
  return 1;
}

static setup_info *get_setup_ptr(u8x8_t *u8x8)
{
   return u8x8_GetUserPtr(u8x8);
}


uint8_t u8x8_d_sdl(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                          void *arg_ptr)
{
  uint8_t x, y, c;
  uint8_t *ptr;
  setup_info *setup = NULL;
  switch (msg) {
  case U8X8_MSG_DISPLAY_SETUP_MEMORY:
    setup = get_setup_ptr(u8x8);
    u8x8_d_helper_display_setup_memory(u8x8, &setup->u8x8_sdl_info);
    u8g_sdl_init(setup->width, setup->height);
    break;
  case U8X8_MSG_DISPLAY_INIT:
    u8x8_d_helper_display_init(u8x8);
    break;
  case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
    break;
  case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
    break;
  case U8X8_MSG_DISPLAY_SET_CONTRAST:
    break;
  case U8X8_MSG_DISPLAY_DRAW_TILE:
    x = ((u8x8_tile_t *)arg_ptr)->x_pos;
    x *= 8;
    x += u8x8->x_offset;

    y = ((u8x8_tile_t *)arg_ptr)->y_pos;
    y *= 8;

    do {
      c = ((u8x8_tile_t *)arg_ptr)->cnt;
      ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
      u8g_sdl_set_multiple_8pixel(x, y, c * 8, ptr);
      arg_int--;
      // x+=c*8;
    } while (arg_int > 0);

    /* update all */
    SDL_UpdateWindowSurface(u8g_sdl_window);
    break;
  default:
    return 0;
  }
  return 1;
}

void u8x8_Setup_SDL(u8x8_t *u8x8, setup_info *setup)
{
  /* setup defaults */
  u8x8_SetupDefaults(u8x8);

  /* setup specific callbacks */
  u8x8_SetUserPtr(u8x8, setup);
  u8x8->display_cb = u8x8_d_sdl;

  u8x8->gpio_and_delay_cb = u8x8_d_sdl_gpio;

  /* setup display info */
  u8x8_SetupMemory(u8x8);
}

int u8g2_SetupBuffer_SDL(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb, int width, int height)
{
  setup_info *setup = (setup_info *) malloc(sizeof(setup_info));
  setup_info_init(setup, width, height);
  u8x8_Setup_SDL(u8g2_GetU8x8(u8g2), setup);
  /*
  fprintf(stderr, "w=%d, h=%d\n", setup->width, setup->height);
  fprintf(stderr, "pixw=%d, pixh=%d\n", setup->u8x8_sdl_info.pixel_width,
    setup->u8x8_sdl_info.pixel_height);
  fprintf(stderr, "tilew=%d, tileh=%d\n", setup->u8x8_sdl_info.tile_width,
    setup->u8x8_sdl_info.tile_height);
  fprintf(stderr, "sizeof(buf) = %lu\n", sizeof setup->buf);
  exit(0);
  */
  u8g2_SetupBuffer(u8g2, setup->buf, setup->u8x8_sdl_info.tile_height,
                   u8g2_ll_hvline_vertical_top_lsb, u8g2_cb);
  return 1; // success
}

void u8g2_SetupBuffer_SDL_Free(u8g2_t *u8g2)
{
  u8x8_t *u8x8 = u8g2_GetU8x8(u8g2);
  setup_info *setup = get_setup_ptr(u8x8);
  if(setup != NULL) {
    free(setup->buf);
    free(setup);
    u8x8_SetUserPtr(u8x8, NULL);
  }
}

#else // ENABLE_SDL
int u8g2_SetupBuffer_SDL(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb, int width, int height)
{
  // suppress unused parameter warnings
  (void) u8g2;
  (void) u8g2_cb;
  (void) width;
  (void) height;

  return 0; // failure (unsupported)
}

void u8g2_SetupBuffer_SDL_Free(u8g2_t *u8g2)
{
  // suppress unused parameter warnings
  (void) u8g2;
}

#endif // ENABLE_SDL


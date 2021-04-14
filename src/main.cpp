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


#include "u8x8_d_sdl.h"

#include "controller.h"
#include "display.h"
#include "display_info.h"
#include "player.h"
#include "programopts.h"
#include "timer.h"
#include "utils.h"

#include <errno.h>
#include <locale.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <math.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

const int SPECT_WIDTH = 64;

U8G2 *p_u8g2 = nullptr;  // for use during signal handling

void cleanup(void)
{
  // Clear and close display
  if(p_u8g2)
    p_u8g2->clearDisplay();
#ifdef ENABLE_SDL
  SDL_Quit();
#endif
}

void signal_handler(int sig)
{
  switch (sig) {
  case SIGINT:
  case SIGHUP:
  case SIGTERM:
    cleanup();
    break;
  }
  exit(0);
  //abort();
}

void init_signals(void)
{
  struct sigaction new_action;
  memset(&new_action, 0, sizeof(new_action));
  new_action.sa_handler = &signal_handler;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;

  struct sigaction old_action;
  sigaction(SIGINT, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction(SIGINT, &new_action, NULL);
  sigaction(SIGHUP, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction(SIGHUP, &new_action, NULL);
  sigaction(SIGTERM, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction(SIGTERM, &new_action, NULL);
}

class OledOpts : public ProgramOpts {
public:
  const double DEF_SCROLL_RATE = 8;    // pixels per second
  const double DEF_SCROLL_DELAY = 5;   // second delay before scrolling
  string oled;                         // OLED type, as string
  int framerate = 15;                  // frame rate in Hz
  int bars = 16;                       // number of bars in spectrum
  int gap = 1;                         // gap between bars, in pixels
  vector<double> scroll;   // rate (pixels per sec), start delay (secs)
  int clock_format = 0;    // 0-3: 0,1 - 24h  2,3 - 12h  0,2 - leading 0
  int date_format = 0;     // 0: DD-MM-YYYY, 1: MM-DD-YYYY
  char pause_screen = 'p'; // p - play, s - stop
  string cava_prog_name = "mpd_oled_cava"; // cava executable name
  string cava_method = "fifo";             // fifo, alsa or pulse
  string cava_source;                      // Path to FIFO / alsa device
  double cava_start_delay = 2;   // delay (secs) after play before starting cava
  double invert = 0;             // 0 normal, -1 invert, n>0 invert every n hrs
  Player player;

  OledOpts() : ProgramOpts("mpd_oled", "0.02")
  {
    cava_source = "/tmp/" + get_program_name() + "_fifo";
    scroll.push_back(DEF_SCROLL_RATE);
    scroll.push_back(DEF_SCROLL_DELAY);
    scroll.push_back(DEF_SCROLL_RATE);
    scroll.push_back(DEF_SCROLL_DELAY);

    player.init_detect();
  }
  void process_command_line(int argc, char **argv);
  void usage();
  void print_oled_list();
};

void OledOpts::usage()
{
  fprintf(stdout, R"(
Usage: %s -o oled_type [options] [input_file]

Display information about an MPD-based player on an OLED screen

Options
%s
  -o <conf>  OLED configuration in form
                CONTROLLER,MODEL,PROTOCOL[,opt1=val1,opt2=val2,...]
             ('-o help' will print all supported OLEDs and their
             configuration options)
  -b <num>   number of bars to display (default: 16)
  -g <sz>    gap between bars in, pixels (default: 1)
  -f <hz>    framerate in Hz (default: 15)
  -s <vals>  scroll rate (pixels per second) and start delay (seconds), up
             to four comma separated decimal values (default: %.1f,%.1f) as:
                rate_all
                rate_all,delay_all
                rate_title,delay_all,rate_artist
                rate_title,delay_title,rate_artist,delay_artist
  -C <fmt>   clock format: 0 - 24h leading 0 (default), 1 - 24h no leading 0,
                2 - 24h leading 0, 3 - 24h no leading 0
  -d         use USA date format MM-DD-YYYY (default: DD-MM-YYYY)
  -P <val>   pause screen type: p - play (default), s - stop
  -k         cava executable name is cava (default: mpd_oled_cava)
  -c         cava input method and source (default: '%s,%s')
             e.g. 'fifo,/tmp/my_fifo', 'alsa,hw:5,0', 'pulse'
  -I <val>   invert black/white: n - normal (default), i - invert,
             number - switch between n and i with this period (hours), which
             may help avoid screen burn
  -p <plyr>  Player: mpd, moode, volumio, runeaudio (default: detected)
)",
          get_program_name().c_str(), help_ver_text,
          DEF_SCROLL_RATE, DEF_SCROLL_DELAY, cava_method.c_str(),
          cava_source.c_str());
}

void OledOpts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  int c;
  int method_len;

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv, ":ho:b:g:f:s:C:dP:kc:I:p:")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {
    case 'o':
      oled = optarg;
      if (oled == "help") {
         print_oled_list();
         exit(0);
      }

      break;

    case 'b':
      print_status_or_exit(read_int(optarg, &bars), c);
      if (bars < 2 || bars > 60)
        error("select between 2 and 60 bars", c);
      break;

    case 'g':
      print_status_or_exit(read_int(optarg, &gap), c);
      if (gap < 0 || gap > 30)
        error("gap must be between 0 and 30 pixels", c);
      break;

    case 'f':
      print_status_or_exit(read_int(optarg, &framerate), c);
      if (framerate < 1)
        error("framerate must be a positive integer", c);
      break;

    case 's':
      print_status_or_exit(read_double_list(optarg, scroll, 4), c);
      if (scroll.size() < 1)
        scroll.push_back(DEF_SCROLL_RATE);
      else if (scroll[0] < 0)
        error("scroll rate cannot be negative", c);

      if (scroll.size() < 2)
        scroll.push_back(DEF_SCROLL_DELAY);
      else if (scroll[1] < 0)
        error("scroll delay cannot be negative", c);

      if (scroll.size() < 3)
        scroll.push_back(scroll[0]);
      else if (scroll[2] < 0)
        error("scroll rate (origin/artist) cannot be negative", c);

      if (scroll.size() < 4)
        scroll.push_back(scroll[1]);
      else if (scroll[3] < 0)
        error("scroll delay (origin/artist) cannot be negative", c);

      break;

    case 'C':
      print_status_or_exit(read_int(optarg, &clock_format), c);
      if (clock_format < 0 || clock_format > 3)
        error("clock format number is not 0, 1, 2 or 3", c);
      break;

    case 'd':
      date_format = 1;
      break;

    case 'P':
      if (strcmp(optarg, "p") == 0)
        pause_screen = 'p';
      else if (strcmp(optarg, "s") == 0)
        pause_screen = 's';
      else
        error("pause screen type is not p or s", c);
      break;

    case 'k':
      cava_prog_name = "cava";
      break;

    case 'c':
      method_len = 5; // all the initial method strings are length 5!
      if (strncmp(optarg, "fifo,", method_len) == 0) {
        cava_method = "fifo";
        if (optarg[method_len] == '\0')
          error("cava input method is fifo, but no FIFO path was specified", c);
      }
      else if (strncmp(optarg, "alsa,", method_len) == 0) {
        cava_method = "alsa";
        if (optarg[method_len] == '\0')
          error("cava input method is alsa, but no ALSA stream was specified",
                c);
      }
      else if (strncmp(optarg, "pulse", method_len) == 0) {
        cava_method = "pulse";
        if (optarg[method_len] != '\0')
          error("cava input method is pulse, but is followed by extra text", c);
      }
      else
        error("cava input specifier is not in form 'fifo,fifo_path', "
              "'alsa,alsa_stream', or 'pulse'",
              c);

      cava_source = &optarg[method_len];
      break;

    case 'I':
      if (strcmp(optarg, "n") == 0)
        invert = 0;
      else if (strcmp(optarg, "i") == 0)
        invert = -1;
      else if (read_double(optarg, &invert)) {
        if (invert <= 0)
          error("number of hours for period must be positive number", c);
      }
      else
        error("invalid value, should be n, i or a positive number", c);
      break;

    case 'p': {
      // const char *params = "mpd|moode|volumio|runeaudio\n";
      string params = Player::all_names("|");
      string arg_id;
      if (!get_arg_id(optarg, &arg_id, params.c_str()))
        error(msg_str("unknown player name '%s'", optarg).c_str(), c);
      player.set_name(arg_id);
      break;
    }

    default:
      error("unknown command line error");
    }
  }

  if (argc - optind > 0)
    error(msg_str("invalid option or parameter: '%s'", argv[optind]));

  if (oled == "")
    error("must specify an OLED type (-o list for a list)", 'o');

  const int min_spect_width = bars + (bars - 1) * gap; // assume bar width = 1
  if (min_spect_width > SPECT_WIDTH)
    error(msg_str(
        "spectrum graph width is %d: to display %d bars with a gap of %d\n"
        "requires a minimum width of %d. Reduce the number of bars and/or the "
        "gap\n",
        SPECT_WIDTH, bars, gap, min_spect_width));
}

void OledOpts::print_oled_list()
{
  printf(R"(
CONTROLLER SETUP DETAILS

  PROTOCOL KEY
    I2C                    - I2C (alias of HW_I2C), HW_I2C, SW_I2C
    SPI (4 wire SPI)       - SPI (alias of 4W_HW_SPI), 4W_HW_SPI, 4W_SW_SPI
    3W_SW_SPI (3 wire SPI) - 3W_SW_SPI
    8080                   - 8080

)");

  printf("  %-15s%-20s%s\n", "CONTROLLER", "MODEL", "PROTOCOLS");
  printf("  %-15s%-20s%s\n", "----------", "-----", "---------");

  auto setup = ControllerSetup::get_controllers();
  for (auto &kp0 : setup) {
    bool first_model = true;
    printf("\n");
    for (auto &kp1 : kp0.second) {
      std::set<string> protocols;
      for (auto &kp2 : kp1.second) {
        auto com = kp2.first;
        if (com == "I2C" || com == "HW_I2C" || com == "SW_I2C")
          protocols.insert("aI2C");
        else if (com == "SPI" || com == "4W_HW_SPI" || com == "4W_SW_SPI")
          protocols.insert("bSPI");
        else
          protocols.insert("c" + com); // 3W_SW_SPI, 8080
      }
      string protos;
      for (auto proto : protocols)
        protos += proto.substr(1) + ",";
      if (!protos.empty())
        protos.pop_back();

      fprintf(stderr, "  %-15s%-20s%s\n",
              (first_model) ? kp0.first.c_str() : "", kp1.first.c_str(),
              protos.c_str());
      first_model = false;
    }
  }

  printf("\nPROTOCOL OPTIONS\n\n");
  printf("  I2C is an alias for HW_I2C, SPI is an alias for 4W_HW_SPI\n");
  for (string com_str :
       {"HW_I2C", "SW_I2C", "4W_HW_SPI", "4W_SW_SPI", "3W_SW_SPI", "8080"}) {
    int com_id = ControllerSetup::get_com_id(com_str);
    auto com_details = ControllerSetup::get_com_details(com_id);
    printf("    %s - %s\n", com_str.c_str(), com_details->desc.c_str());
    int cnt = 0;
    for (auto val_ids :
         {com_details->values_required, com_details->values_optional}) {
      string str;

      for (int val_id : val_ids) {
        auto value_details = ControllerSetup::get_value_details(val_id);
        str += value_details->id + ",";
      }
      if (!str.empty())
        str.pop_back();
      printf("      %s: %s\n", (cnt == 0) ? "required" : "optional",
             str.c_str());
      cnt++;
    }
  }

  printf("\nPROTOCOL OPTION DESCRIPTIONS\n\n");
  for (int val_id = 0; val_id < ControllerSetup::VAL_LAST; val_id++) {
    auto value_details = ControllerSetup::get_value_details(val_id);
    printf("  %-15s - %s\n", value_details->id.c_str(),
           value_details->desc.c_str());
  }

  printf("\n");
}

string print_config_file(int bars, int framerate, string cava_method,
                         string cava_source, string fifo_path_cava_out)
{
  char templt[] = "/tmp/cava_config_XXXXXX";
  int fd = mkstemp(templt);
  if (fd == -1)
    return ""; // failed to open file and convert to file stream
  FILE *ofile = fdopen(fd, "w");
  if (ofile == NULL)
    return ""; // failed to open file and convert to file stream

  fprintf(ofile,
          "[general]\n"
          "framerate = %d\n"
          "bars = %d\n"
          "\n"
          "[input]\n"
          "method = %s\n"
          "source = %s\n"
          "\n"
          "[output]\n"
          "method = raw\n"
          "data_format = binary\n"
          "channels = mono\n"
          "raw_target = %s\n"
          "bit_format = 8bit\n",
          framerate, bars, cava_method.c_str(), cava_source.c_str(),
          fifo_path_cava_out.c_str());
  fclose(ofile);
  return templt;
}

Status start_cava(FILE **p_fifo_file, const OledOpts &opts)
{

  // Create a FIFO for cava to write its raw output to
  const string fifo_path_cava_out = msg_str("/tmp/cava_fifo_%d", getpid());
  unlink(fifo_path_cava_out.c_str());
  if (mkfifo(fifo_path_cava_out.c_str(), 0666) == -1)
    opts.error("could not create cava output FIFO for writing: " +
               string(strerror(errno)));

  // Create a temporary config file for cava
  string config_file_name =
      print_config_file(opts.bars, opts.framerate, opts.cava_method,
                        opts.cava_source, fifo_path_cava_out);
  if (config_file_name == "")
    opts.error("could not create cava config file: " + string(strerror(errno)));

  // Create a pipe to a cava subprocess
  string cava_cmd = opts.cava_prog_name + " -p " + config_file_name;
  if (popen(cava_cmd.c_str(), "r") == NULL)
    opts.error("could not start cava program: " + string(strerror(errno)));

  // Create a file stream to read cava's raw output from
  *p_fifo_file = fopen(fifo_path_cava_out.c_str(), "rb");
  if (*p_fifo_file == NULL)
    opts.error("could not open cava output FIFO for reading");

  return Status::ok();
}

// Draw fullscreen 128x64 clock/date
void draw_clock(U8G2 &u8g2, const display_info &disp_info)
{
  draw_text(u8g2, 22, 0, disp_info.conn.get_ip_addr(), u8g2_font_courB08_tf);
  draw_connection(u8g2, 116, 0, disp_info.conn);
  draw_time(u8g2, 10, 16, disp_info.clock_format, u8g_font_courB24);
  draw_date(u8g2, 32, 52, disp_info.date_format, u8g_font_courB08);
}

void draw_spect_display(U8G2 &u8g2, const display_info &disp_info)
{
  draw_spectrum(u8g2, 0, 0, SPECT_WIDTH, 32, disp_info.spect);
  draw_connection(u8g2, 116, 0, disp_info.conn);
  draw_triangle_slider(u8g2, 100, 0, 12, 8, disp_info.status.get_volume());

    //draw_text(u8g2, 68, 0, disp_info.status.get_kbitrate_str().c_str());
  if (disp_info.status.get_kbitrate() > 0)
    draw_text(u8g2, 68, 0, disp_info.status.get_kbitrate_str(), u8g_font_courB08);

  u8g2.setMaxClipWindow();
  int clock_offset = (disp_info.clock_format < 2) ? 0 : -2;
  draw_time(u8g2, 72 + clock_offset, 16, disp_info.clock_format,
            u8g2_font_courB14_tf);

  vector<double> scroll_origin(disp_info.scroll.begin() + 2,
                               disp_info.scroll.begin() + 4);
  draw_text_scroll(u8g2, 0, 38, disp_info.status.get_origin(), scroll_origin,
                   disp_info.text_change.secs(), u8g_font_courB08, 128, 10);

  vector<double> scroll_title(disp_info.scroll.begin(),
                              disp_info.scroll.begin() + 2);
  draw_text_scroll(u8g2, 0, 46, disp_info.status.get_title(), scroll_title,
                   disp_info.text_change.secs(), u8g_font_courB08, 128, 10);

  draw_solid_slider(u8g2, 0, 56 + 6, 128, 2,
                    100 * disp_info.status.get_progress());
}

void draw_display(U8G2 &u8g2, const display_info &disp_info)
{
  u8g2.setFontPosTop();
  mpd_state state = disp_info.status.get_state();
  if (state == MPD_STATE_UNKNOWN || state == MPD_STATE_STOP ||
      (state == MPD_STATE_PAUSE && disp_info.pause_screen == 's'))
    draw_clock(u8g2, disp_info);
  else
    draw_spect_display(u8g2, disp_info);
}

namespace {
pthread_mutex_t disp_info_lock;
}

void *update_info(void *data)
{
  const float delay_secs = 0.3;
  display_info *disp_info_orig = (display_info *)data;
  while (true) {
    pthread_mutex_lock(&disp_info_lock);
    display_info disp_info = *disp_info_orig;
    pthread_mutex_unlock(&disp_info_lock);

    disp_info.status.init(); // Update MPD status info
    disp_info.conn_init();   // Update connection info

    pthread_mutex_lock(&disp_info_lock);
    disp_info_orig->update_from(disp_info);
    pthread_mutex_unlock(&disp_info_lock);

    usleep(delay_secs * 1000000);
  }
};

bool get_invert(double period)
{
  return (period > 0) ? (fmod(time(0) / 3600.0, 2 * period) > period) : period;
}

int start_idle_loop(U8G2 *p_u8g2, const OledOpts &opts)
{
  auto &u8g2 = *p_u8g2;
  const double update_sec =
      1 / (0.9 * opts.framerate); // default update freq just under framerate
  const long select_usec =
      update_sec * 1100000; // slightly longer, but still less than framerate
  Timer timer;

  display_info disp_info;
  disp_info.scroll = opts.scroll;
  disp_info.clock_format = opts.clock_format;
  disp_info.date_format = opts.date_format;
  disp_info.pause_screen = opts.pause_screen;
  disp_info.spect.init(opts.bars, opts.gap);
  disp_info.status.set_player(opts.player);
  disp_info.status.init();

  // Update MPD info in separate thread to avoid stuttering in the spectrum
  // animation.
  pthread_t update_info_thread;
  if (pthread_create(&update_info_thread, NULL, update_info,
                     (void *)(&disp_info))) {
    fprintf(stderr, "error: could not create pthread\n");
    return 1;
  }

  if (pthread_mutex_init(&disp_info_lock, NULL) != 0) {
    fprintf(stderr, "error: could not create pthread mutex\n");
    return 2;
  }

  // Cava not yet started
  int fifo_fd = -1;
  FILE *fifo_file = nullptr;

  while (true) {
    int num_bars_read = 0;
    if (fifo_fd >= 0) {
      fd_set set;
      FD_ZERO(&set);
      FD_SET(fifo_fd, &set);

      // FIFO read timeout value
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = select_usec; // slightly longer than timer

      // If there is data read it all.
      if (select(FD_SETSIZE, &set, NULL, NULL, &timeout) > 0) {
        do {
          num_bars_read =
              fread(&disp_info.spect.heights[0], sizeof(unsigned char),
                    disp_info.spect.heights.size(), fifo_file);

          FD_ZERO(&set);
          FD_SET(fifo_fd, &set);
          timeout.tv_sec = 0;
          timeout.tv_usec = 0;
        } while (select(FD_SETSIZE, &set, NULL, NULL, &timeout) > 0);
      }
    }

    // Clear spectrum data if no data read or music not playing
    if (num_bars_read == 0 || disp_info.status.get_state() != MPD_STATE_PLAY) {
      std::fill(disp_info.spect.heights.begin(), disp_info.spect.heights.end(),
                0);
      usleep(0.1 * 1000000); // 0.1 sec delay, don't idle too fast if no need
    }

    // Update display if necessary
    if (timer.finished() || num_bars_read) {
      u8g2.clearBuffer();
      pthread_mutex_lock(&disp_info_lock);
      // u8g2.invertDisplay(get_invert(opts.invert));
      draw_display(u8g2, disp_info);
      pthread_mutex_unlock(&disp_info_lock);
      u8g2.sendBuffer();
    }

    if (timer.finished()) {
      //u8g2.initDisplay();   // reset offset (but turns on power save)
      //u8g2.setPowerSave(0); // turn off power save
      if (disp_info.status.get_state() == MPD_STATE_PLAY && fifo_fd < 0) {
        usleep(opts.cava_start_delay*1000000);
        opts.print_status_or_exit(start_cava(&fifo_file, opts));
        fifo_fd = fileno(fifo_file);
      }

      timer.set_timer(update_sec); // Reset the timer
    }
  }

  return 0;
}


std::vector<std::string> split(const std::string &text,
                               const std::string &delims)
{
  std::vector<std::string> tokens;
  std::size_t start = text.find_first_not_of(delims), end = 0;

  while ((end = text.find_first_of(delims, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = text.find_first_not_of(delims, end);
  }
  if (start != std::string::npos)
    tokens.push_back(text.substr(start));

  return tokens;
}

Status init_display(U8G2 **pp_u8g2, string oled)
{
  auto settings = split(oled, ",");
  if(settings.size() == 0) {
    return Status::error("must specify a controller");
  }
  if(settings[0] == "SDL") {
    if(settings.size() > 2)
      return Status::error(
          "controller SDL: extra parameters specified (use SDL,WXH)");
    string dim_string = settings.size() == 1 ? "128X64" : settings[1];
    auto dims = split(dim_string, "X");
    if(dims.size() != 2)
      return Status::error("controller SDL: dimensions is not in form WxH");
    int width;
    if(!read_int(dims[0].c_str(), &width))
      return Status::error("controller SDL: width dimension is not a number");
    int height;
    if(!read_int(dims[1].c_str(), &height))
      return Status::error("controller SDL: height dimension is not a number");

    *pp_u8g2 = new U8G2;
    if (u8g2_SetupBuffer_SDL((*pp_u8g2)->getU8g2(), U8G2_R0, width, height)) {
      u8x8_InitDisplay((*pp_u8g2)->getU8x8());
      u8x8_SetPowerSave((*pp_u8g2)->getU8x8(), 0);
    }
    else {
      delete *pp_u8g2;
      *pp_u8g2 =  nullptr;
      return Status::error("controller SDL: SDL support was not included");
    }
  }
  else {
    if (settings.size() < 3) {
      settings.resize(3, string("not specified"));
      //  fprintf(stderr, "error: did find three controller parameters
      //  CONTROLLER_TYPE,DISPLAY_NAME,COM_TYPE\n"); exit(1);
    }
    ControllerSetup setup;

    string errmsg;
    if (!setup.set_controller(settings[0], settings[1], settings[2], errmsg)) 
      return Status::error(errmsg);

    for (size_t i = 3; i < settings.size(); i++) {
      errmsg = set_setup_value(setup, settings[i]);
      if (!errmsg.empty())
        return Status::error(errmsg);
    }

    *pp_u8g2 = new U8G2;
    errmsg = setup.init(*pp_u8g2);
    if (!errmsg.empty()) {
      delete *pp_u8g2;
      *pp_u8g2 =  nullptr;
      return Status::error("controller type '" + settings[0] + "', name '" +
                           settings[1] + "', protocol '" + settings[2] +
                           "': " + errmsg);
    }
  }

  return Status::ok();
}


int main(int argc, char **argv)
{
  // Set locale to allow iconv transliteration to US-ASCII
  setlocale(LC_CTYPE, "C.UTF-8");
  OledOpts opts;
  opts.process_command_line(argc, argv);

  // Set up the OLED display
  opts.print_status_or_exit(init_display(&p_u8g2, opts.oled), 'o');

  p_u8g2->begin();

  init_signals();
  atexit(cleanup);
  int loop_ret = start_idle_loop(p_u8g2, opts);

  if (loop_ret != 0)
    exit(EXIT_FAILURE);

  return 0;
}

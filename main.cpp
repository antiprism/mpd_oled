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
#include "display_info.h"
#include "timer.h"
#include "programopts.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <errno.h>

#include <math.h>
#include <vector>
#include <string>

using std::vector;
using std::string;

const int SPECT_WIDTH = 64;

ArduiPi_OLED display; // global, for use during signal handling

void cleanup(void)
{
  // Clear and close display
  display.clearDisplay();
  display.display();
  display.close();
}

void signal_handler(int sig)
{
  switch(sig) {
    case SIGINT:
    case SIGHUP:
    case SIGTERM:
      cleanup();
      break;
  }
}

void init_signals(void)
{
  struct sigaction new_action;
  memset(&new_action, 0, sizeof(new_action));
  new_action.sa_handler = &signal_handler;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;

  struct sigaction old_action;
  sigaction (SIGINT, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGINT, &new_action, NULL);
  sigaction (SIGHUP, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGHUP, &new_action, NULL);
  sigaction (SIGTERM, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGTERM, &new_action, NULL);
}

class OledOpts : public ProgramOpts {
public:
  const double DEF_SCROLL_RATE;   // pixels per second
  const double DEF_SCROLL_DELAY;  // second delay before scrolling
  string version;
  int oled;                       // OLED type, as a number
  int framerate;                  // frame rate in Hz
  int bars;                       // number of bars in spectrum
  int gap;                        // gap between bars, in pixels
  vector<double> scroll;          // rate (pixels per sec), start delay (secs)
  int clock_format;               // 0-3: 0,1 - 24h  2,3 - 12h  0,2 - leading 0
  string cava_method;             // fifo, alsa or pulse
  string cava_source;             // Path to FIFO for audio-in, alsa device...
  bool rotate180;                 // display upside down
  unsigned char i2c_addr;         // number of I2C address
  int reset_gpio;
  int source;

  OledOpts(): ProgramOpts("mpd_oled", "0.01"),
      DEF_SCROLL_RATE(8.0),
      DEF_SCROLL_DELAY(5.0),
      oled(OLED_ADAFRUIT_SPI_128x32),
      framerate(15),
      bars(16),
      gap(1),
      clock_format(0),
      cava_method("fifo"),
      cava_source("/tmp/mpd_oled_fifo"),
      rotate180(false),
      i2c_addr(0),
      reset_gpio(25),
      // Default for source of status values depends on the player
      source(
#ifdef VOLUMIO
        mpd_info::SOURCE_VOLUMIO
#else
        mpd_info::SOURCE_MPD
#endif
      )
      {
        scroll.push_back(DEF_SCROLL_RATE);
        scroll.push_back(DEF_SCROLL_DELAY);
        scroll.push_back(DEF_SCROLL_RATE);
        scroll.push_back(DEF_SCROLL_DELAY);
      }
  void process_command_line(int argc, char **argv);
  void usage();
};

// clang-format off
void OledOpts::usage()
{
   fprintf(stdout,
"\n"
"Usage: %s -o oled_type [options] [input_file]\n"
"\n"
"Display information about an MPD-based player on an OLED screen\n"
"\n"
"Options\n"
"%s", get_program_name().c_str(), help_ver_text);

   fprintf(stdout,
"  -o <type>  OLED type, specified as a number, from the following:\n");
  for (int i=0; i<OLED_LAST_OLED;i++)
    if (strstr(oled_type_str[i], "128x64"))
      fprintf(stdout, "      %1d %s\n", i, oled_type_str[i]);

   fprintf(stdout,
"  -b <num>   number of bars to display (default: 16)\n"
"  -g <sz>    gap between bars in, pixels (default: 1)\n"
"  -f <hz>    framerate in Hz (default: 15)\n"
"  -s <vals>  scroll rate (pixels per second) and start delay (seconds), up\n"
"             to four comma separated decimal values (default: %.1f,%.1f) as:\n"
"                rate_all\n"
"                rate_all,delay_all\n"
"                rate_title,delay_all,rate_artist\n"
"                rate_title,delay_title,rate_artist,delay_artist\n"
"  -C <fmt>   clock format: 0 - 24h leading 0 (default), 1 - 24h no leading 0,\n"
"                2 - 24h leading 0, 3 - 24h no leading 0\n"
"  -c         cava input method and source (default: '%s,%s')\n"
"             e.g. 'fifo,/tmp/my_fifo', 'alsa,hw:5,0', 'pulse'\n"
"  -R         rotate display 180 degrees\n"
"  -a <addr>  I2C address, in hex (default: default for OLED type)\n"
"  -r <gpio>  I2C reset GPIO number, if needed (default: 25)\n"
"Example :\n"
"%s -o 6 use a %s OLED\n"
"\n",
      DEF_SCROLL_RATE, DEF_SCROLL_DELAY,
      cava_method.c_str(), cava_source.c_str(),
      get_program_name().c_str(), oled_type_str[6]);
}
// clang-format on

void OledOpts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  int c;
  int method_len;

  handle_long_opts(argc, argv);

  while ((c=getopt(argc, argv, ":ho:b:g:f:s:C:c:Ra:r:")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {
    case 'o':
      print_status_or_exit(read_int(optarg, &oled), c);
      if (oled < 0 || oled >= OLED_LAST_OLED ||
          !strstr(oled_type_str[oled], "128x64"))
        error(msg_str("invalid 128x64 oled type %d (see -h)", oled), c);
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
      if(clock_format < 0 || clock_format > 3)
        error("clock format number is not 0, 1, 2 or 3", c);
      break;

    case 'c':
      method_len = 5;      // all the initial method strings are length 5!
      if (strncmp(optarg, "fifo,", method_len) == 0) {
        cava_method = "fifo";
        if(optarg[method_len] == '\0')
          error("cava input method is fifo, but no FIFO path was specified", c);
      }
      else if (strncmp(optarg, "alsa,", method_len) == 0) {
        cava_method = "alsa";
        if(optarg[method_len] == '\0')
          error("cava input method is alsa, but no ALSA stream was specified",
              c);
      }
      else if (strncmp(optarg, "pulse", method_len) == 0) {
        cava_method = "pulse";
        if(optarg[method_len] != '\0')
          error("cava input method is pulse, but is followed by extra text", c);
      }
      else
        error("cava input specifier is not in form 'fifo,fifo_path', "
              "'alsa,alsa_stream', or 'pulse'", c);

      cava_source = &optarg[method_len];
      break;

    case 'R':
      rotate180 = true;
      break;

    case 'a':
      if (strlen(optarg) != 2 ||
          strspn(optarg, "01234567890aAbBcCdDeEfF") != 2 )
        error("I2C address should be two hexadecimal digits", c);

      i2c_addr = (unsigned char) strtol(optarg, NULL, 16);
      break;

    case 'r':
      print_status_or_exit(read_int(optarg, &reset_gpio), c);
      if (!isdigit(optarg[0]) || reset_gpio < 0 || reset_gpio > 99)
        error("probably invalid (not integer in range 0 - 99), specify the\n"
              "GPIO number of the pin that RST is connected to", c);
      break;

    default:
      error("unknown command line error");
    }
  }

  if (oled == 0)
    error("must specify a 128x64 oled", 'o');
  
  const int min_spect_width = bars + (bars-1)*gap; // assume bar width = 1
  if (min_spect_width > SPECT_WIDTH)
     error(msg_str(
"spectrum graph width is %d: to display %d bars with a gap of %d\n"
"requires a minimum width of %d. Reduce the number of bars and/or the gap\n",
         SPECT_WIDTH, bars, gap, min_spect_width));
}

string print_config_file(int bars, int framerate,
    string cava_method, string cava_source, string fifo_path_cava_out)
{
  char templt[] = "/tmp/cava_config_XXXXXX";
  int fd = mkstemp(templt);
  if(fd == -1)
    return "";  // failed to open file and convert to file stream
  FILE *ofile = fdopen(fd, "w");
  if(ofile == NULL)
    return "";  // failed to open file and convert to file stream

  fprintf(ofile, "[general]\n"
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

// Draw fullscreen 128x64 clock/date
void draw_clock(ArduiPi_OLED &display, const display_info &disp_info)
{
  display.clearDisplay();
  const int H = 8;  // character height
  const int W = 6;  // character width
  draw_text(display, 22, 0, 16, disp_info.conn.get_ip_addr());
  draw_connection(display, 128-2*W, 0, disp_info.conn);
  draw_time(display, 4, 16, 4, disp_info.clock_format);
  draw_date(display, 32, 56, 1);
}


void draw_spect_display(ArduiPi_OLED &display, const display_info &disp_info)
{
  const int H = 8;  // character height
  const int W = 6;  // character width
  draw_spectrum(display, 0, 0, SPECT_WIDTH, 32, disp_info.spect);
  draw_connection(display, 128-2*W, 0, disp_info.conn);
  draw_triangle_slider(display, 128-5*W, 1, 11, 6, disp_info.status.get_volume());
  if (disp_info.status.get_kbitrate() > 0)
    draw_text(display, 128-10*W, 0, 4, disp_info.status.get_kbitrate_str());

  int clock_offset = (disp_info.clock_format < 2) ? 0 : -2;
  draw_time(display, 128-10*W+clock_offset, 2*H, 2, disp_info.clock_format);

  vector<double> scroll_origin(disp_info.scroll.begin()+2,
                               disp_info.scroll.begin()+4);
  draw_text_scroll(display, 0, 4*H+4, 20, disp_info.status.get_origin(),
      scroll_origin, disp_info.text_change.secs());

  vector<double> scroll_title(disp_info.scroll.begin(),
                              disp_info.scroll.begin()+2);
  draw_text_scroll(display, 0, 6*H, 20, disp_info.status.get_title(),
      scroll_title, disp_info.text_change.secs());

  draw_solid_slider(display, 0, 7*H+6, 128, 2,
      100*disp_info.status.get_progress());
}


void draw_display(ArduiPi_OLED &display, const display_info &disp_info)
{
  if (disp_info.status.get_state() == MPD_STATE_UNKNOWN ||
      disp_info.status.get_state() == MPD_STATE_STOP)
    draw_clock(display, disp_info);
  else
    draw_spect_display(display, disp_info);
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

    disp_info.status.init();          // Update MPD status info
    disp_info.conn_init();            // Update connection info

    pthread_mutex_lock(&disp_info_lock);
    disp_info_orig->update_from(disp_info);
    pthread_mutex_unlock(&disp_info_lock);

    usleep(delay_secs * 1000000);
  }
};


int start_idle_loop(ArduiPi_OLED &display, FILE *fifo_file,
    const OledOpts &opts)
{
  const double update_sec = 1/(0.9*opts.framerate); // default update freq just under framerate
  const long select_usec = update_sec * 1001000;    // slightly longer, but still less than framerate
  int fifo_fd = fileno(fifo_file);
  Timer timer;
  
  display_info disp_info;
  disp_info.scroll = opts.scroll;
  disp_info.clock_format = opts.clock_format;
  disp_info.spect.init(opts.bars, opts.gap);
  disp_info.status.set_source(opts.source);
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

  while (true) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fifo_fd, &set);

    // FIFO read timeout value
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = select_usec;  // slightly longer than timer


    // If there is data read it, otherwise use zero data.

    int num_bars_read = 0;
    if(disp_info.status.get_state() == MPD_STATE_PLAY &&
        select(FD_SETSIZE, &set, NULL, NULL, &timeout) > 0)
      num_bars_read = fread(&disp_info.spect.heights[0], sizeof(unsigned char),
          disp_info.spect.heights.size(), fifo_file);
    else {
      std::fill(disp_info.spect.heights.begin(), disp_info.spect.heights.end(),
          0);
      usleep(0.1 * 1000000);  // 0.1 sec delay, don't idle too fast if no need
    }
    // Update display if necessary
    if (timer.finished() || num_bars_read) {
       display.clearDisplay();
       pthread_mutex_lock(&disp_info_lock);
       draw_display(display, disp_info);
       pthread_mutex_unlock(&disp_info_lock);
       display.display();
    }

    if(timer.finished())
      timer.set_timer(update_sec);   // Reset the timer
  }

  return 0;
}


int main(int argc, char **argv)
{
  // Set locale to allow iconv transliteration to US-ASCII
  setlocale(LC_CTYPE, "en_US.UTF-8");
  OledOpts opts;
  opts.process_command_line(argc, argv);

  // Set up the OLED doisplay
  if(!init_display(display, opts.oled, opts.i2c_addr, opts.reset_gpio,
        opts.rotate180))
    opts.error("could not initialise OLED");

  // Create a FIFO for cava to write its raw output to
  const string fifo_path_cava_out = msg_str("/tmp/cava_fifo_%d", getpid());
  unlink(fifo_path_cava_out.c_str());
  if(mkfifo(fifo_path_cava_out.c_str(), 0666) == -1)
    opts.error("could not create cava output FIFO for writing: " +
               string(strerror(errno)));

  // Create a temporary config file for cava
  string config_file_name = print_config_file(opts.bars, opts.framerate,
      opts.cava_method, opts.cava_source, fifo_path_cava_out);
  if (config_file_name == "")
    opts.error("could not create cava config file: " +
               string(strerror(errno)));

  // Create a pipe to a cava subprocess
  string cava_cmd = "cava -p " + config_file_name;
  FILE *from_cava = popen(cava_cmd.c_str(), "r");
  if (from_cava == NULL)
    opts.error("could not start cava program: " +
               string(strerror(errno)));

  // Create a file stream to read cava's raw output from
  FILE *fifo_file = fopen(fifo_path_cava_out.c_str(), "rb");
  if(fifo_file == NULL)
    opts.error("could not open cava output FIFO for reading");

  init_signals();
  atexit(cleanup);
  int loop_ret = start_idle_loop(display, fifo_file, opts);

  if(loop_ret != 0)
    exit(EXIT_FAILURE);

  return 0;
}

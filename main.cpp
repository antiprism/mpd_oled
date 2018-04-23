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
#include "spect_graph.h"
#include "status.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <getopt.h>

#include <math.h>
#include <vector>
#include <string>

using std::vector;
using std::string;

const int SPECT_WIDTH = 64;

struct display_info
{
  spect_graph spect;
  mpd_info status;
  int conn;
  void conn_init() { conn = get_connection_info(); }
};


struct ProgOpts
{
  string prog_name;
  string version;
  int oled;
  int framerate;
  int bars;
  int gap;
  unsigned char i2c_addr;
  int reset_gpio;

  ProgOpts(): prog_name("mpd_oled"), version("0.01"),
              oled(OLED_ADAFRUIT_SPI_128x32), framerate(15), bars(16), gap(1),
              i2c_addr(0), reset_gpio(25)
              {}
  void usage();
  void parse_args(int argc, char *argv[]);
};


void ProgOpts::usage()
{
  printf("%s\n", prog_name.c_str());
  printf("Usage: %s -o oled_type [options]\n", prog_name.c_str());
  printf("  -o <type>  OLED type\nOLED type are:\n");
  for (int i=0; i<OLED_LAST_OLED;i++)
    if (strstr(oled_type_str[i], "128x64"))
      printf("  %1d %s\n", i, oled_type_str[i]);

  printf("Options:\n");
  printf("  -h         help\n");
  printf("  -b <num>   number of bars to display (default: 16)\n");
  printf("  -g <sz>    gap between bars in, pixels (default: 1)\n");
  printf("  -f <hz>    framerate in Hz (default: 15)\n");
  printf("  -a <addr>  I2C address, in hex (default: default for OLED type)\n");
  printf("  -r <gpio>  I2C reset GPIO number, if needed (default: 25)\n");
  printf("Example :\n");
  printf( "%s -o 6 use a %s OLED\n\n", prog_name.c_str(), oled_type_str[6]);
}


void ProgOpts::parse_args(int argc, char *argv[])
{
  opterr = 1;  // suppress error message for unrecognised option
  int c;
  while ((c=getopt(argc, argv, ":ho:b:g:f:a:r:")) != -1)
  {
    switch (c) 
    {
      case 'o':
        oled = (int) atoi(optarg);
        if (oled < 0 || oled >= OLED_LAST_OLED ||
            !strstr(oled_type_str[oled], "128x64")) {
          fprintf(stderr, "error: -o %d: invalid 128x64 oled type (see -h)\n",
              oled);
          exit(EXIT_FAILURE);
        }
        break;

      case 'b':
        bars = (int) atoi(optarg);
        if (bars < 2 || bars > 60) {
          fprintf(stderr, "error: -b %d: select between 2 and 60 bars\n", bars);
          exit(EXIT_FAILURE);
        }
        break;

      case 'g':
        gap = (int) atoi(optarg);
        if (gap < 0 || gap > 30) {
          fprintf(stderr, "error: -g %d: select gap between 0 and 30 pixels\n",
              gap);
          exit(EXIT_FAILURE);
        }
        break;

      case 'f':
        framerate = (int) atoi(optarg);
        if (framerate < 1) {
          fprintf(stderr,
              "error: -f %d: framerate must be a positive integer\n", framerate);
          exit(EXIT_FAILURE);
        }
        break;

      case 'a':
        if (strlen(optarg) != 2 ||
            strspn(optarg, "01234567890aAbBcCdDeEfF") != 2 ) {
          fprintf(stderr,
              "error: -a %s: I2C address should be two hexadecimal digits\n",
              optarg);
          exit(EXIT_FAILURE);
        }

        i2c_addr = (unsigned char) strtol(optarg, NULL, 16);
        break;

      case 'r':
        reset_gpio = (int) atoi(optarg);
        if (!isdigit(optarg[0]) || reset_gpio < 0 || reset_gpio > 99) {
          fprintf(stderr, "error: -r %s: probably invalid (not integer in "
              "range 0 - 99), specify the\nGPIO number of the pin that RST "
              "is connected to\n", optarg);
          exit(EXIT_FAILURE);
        }
        break;

      case 'h':
        usage();
        exit(EXIT_SUCCESS);
        break;

      
      case '?': 
        fprintf(stderr, "error: unrecognized option -%c.\n", optopt);
        fprintf(stderr, "run with '-h'.\n");
        exit(EXIT_FAILURE);
              
      case ':':
        fprintf(stderr, "error: -%c missing argument.\n", optopt);
        fprintf(stderr, "run with '-h'.\n");
        exit(EXIT_FAILURE);
              
      default:
        fprintf(stderr, "error: unknown error\n");
        fprintf(stderr, "run with '-h'.\n");
        exit(EXIT_FAILURE);
    }
  }

  if (oled == 0) {
    fprintf(stderr, "error: must specify a 128x64 oled with -o\n");
    exit(EXIT_FAILURE);
  }
  
  const int min_spect_width = bars + (bars-1)*gap; // assume bar width = 1
  if (min_spect_width > SPECT_WIDTH) {
    fprintf(stderr,
"error: spectrum graph width is %d: to display %d bars with a gap of %d\n"
"requires a minimum width of %d. Reduce the number of bars and/or the gap.\n",
      SPECT_WIDTH, bars, gap, min_spect_width);
    exit(EXIT_FAILURE);
  }
}

string print_config_file(int bars, int framerate, string fifo_name)
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
                 "method = fifo\n"
                 "source = /tmp/mpd_oled_fifo\n"
                 "\n"
                 "[output]\n"
                 "method = raw\n"
                 "data_format = binary\n"
                 "channels = mono\n"
                 "raw_target = %s\n"
                 "bit_format = 8bit\n",
          framerate, bars, fifo_name.c_str());
  fclose(ofile);
  return templt;
}

// Draw fullscreen 128x64 clock/date
void draw_clock(ArduiPi_OLED &display)
{
  display.clearDisplay();
  draw_time(display, 4, 4, 4, 0);
  draw_time(display, 4, 48, 2, 1);
}


void draw_spect_display(ArduiPi_OLED &display, const display_info &disp_info)
{
  const int H = 8;  // character height
  const int W = 6;  // character width
  draw_spectrum(display, 0, 0, SPECT_WIDTH, 32, disp_info.spect);
  draw_connection(display, 128-2*W, 0, disp_info.conn);
  //draw_slider(display, 128-5*W, 1, 11, 6, disp_info.status.get_volume());
  draw_triangle_slider(display, 128-5*W, 1, 11, 6, disp_info.status.get_volume());
  draw_text(display, 128-11*W, 0, 4, disp_info.status.get_kbitrate_str());
  
  draw_time(display, 128-10*W, 2*H, 2);
  
  draw_text(display, 0, 4*H+4, 20, disp_info.status.get_origin());
  draw_text(display, 0, 6*H, 20, disp_info.status.get_title());
  //draw_slider(display, 0, 7*H+4, 128, 4, 100*disp_info.status.get_progress());
  draw_solid_slider(display, 0, 7*H+6, 128, 2,
      100*disp_info.status.get_progress());
}


void draw_display(ArduiPi_OLED &display, const display_info &disp_info)
{
  if (disp_info.status.get_state() == MPD_STATE_UNKNOWN ||
      disp_info.status.get_state() == MPD_STATE_STOP)
    draw_clock(display);
  else
    draw_spect_display(display, disp_info);
}


void *update_info(void *data)
{
  display_info *disp_info = (display_info *)data;
  const float delay_secs = 0.3;
  while (true) {
    disp_info->status.init();          // Update MPD status info
    disp_info->conn_init();            // Update connection info
    usleep(delay_secs * 1000000);
  }
};


int start_idle_loop(ArduiPi_OLED &display, FILE *fifo_file,
    const ProgOpts &opts)
{
  const double update_sec = 0.9; // default update freq = 10 /s
  const long select_usec = update_sec * 1001000; // slightly longer
  int fifo_fd = fileno(fifo_file);
  Timer timer;
  
  display_info disp_info;
  disp_info.spect.init(opts.bars, opts.gap);
  disp_info.status.init();
 
  // Update MPD info in separate thread to avoid stuttering in the spectrum
  // animation.
  pthread_t update_info_thread;
  pthread_create(&update_info_thread, NULL, update_info, (void *)(&disp_info));
  
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
    if(select(FD_SETSIZE, &set, NULL, NULL, &timeout))
      num_bars_read = fread(&disp_info.spect.heights[0], sizeof(unsigned char),
          disp_info.spect.heights.size(), fifo_file);
    else
      std::fill(disp_info.spect.heights.begin(), disp_info.spect.heights.end(),
          0);

    // Update display if necessary
    if (timer.finished() || num_bars_read) {
       display.clearDisplay();
       draw_display(display, disp_info);
       display.display();
    }

    if(timer.finished())
      timer.set_timer(update_sec);   // Reset the timer
  }
}


int main(int argc, char **argv)
{
  // Set locale to allow iconv transliteration to US-ASCII
  setlocale(LC_CTYPE, "en_US.UTF-8");
  ProgOpts opts;
  opts.parse_args(argc, argv);

  // Set up the OLED doisplay
  ArduiPi_OLED display;
  if(!init_display(display, opts.oled, opts.i2c_addr, opts.reset_gpio)) {
    fprintf(stderr, "error: could not initialise OLED\n");
    exit(EXIT_FAILURE);
  }

  // Create a FIFO for cava to write its raw output to
  const string fifo_name = "/tmp/cava_fifo";
  unlink(fifo_name.c_str());
  if(mkfifo(fifo_name.c_str(), 0666) == -1) {
    perror("could not create cava FIFO for writing");
    exit(EXIT_FAILURE);
  }

  // Create a temporary config file for cava
  string config_file_name = print_config_file(opts.bars, opts.framerate,
      fifo_name);
  if (config_file_name == "") {
    perror("could not create cava config file");
    exit(EXIT_FAILURE);
  }

  // Create a pipe to a cava subprocess
  string cava_cmd = "cava -p " + config_file_name;
  FILE *from_cava = popen(cava_cmd.c_str(), "r");
  if (from_cava == NULL) {
    perror("could not start cava program");
    exit(EXIT_FAILURE);
  }

  // Create a file stream to read cava's raw output from
  FILE *fifo_file = fopen(fifo_name.c_str(), "rb");
  if(fifo_file == NULL) {
    fprintf(stderr, "could not open cava FIFO for reading");
    exit(EXIT_FAILURE);
  }

  start_idle_loop(display, fifo_file, opts);
  //read_and_display_loop(display, fifo_file, type, opts.bars, opts.gaps);
  
  // Free PI GPIO ports
  display.close();

}

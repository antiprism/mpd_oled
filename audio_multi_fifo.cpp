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

#include "programopts.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>

#include <math.h>
#include <vector>
#include <string>

using std::vector;
using std::string;

const char *PROGRAM_NAME = "audio_multi_fifo";
static vector<string> fifo_paths;

class MultiOpts : public ProgramOpts {
public:
  string version;
  string conf_file;

  MultiOpts(): ProgramOpts(PROGRAM_NAME, "0.01"),
      conf_file(string(PROGRAM_NAME)+".conf")
      {}
  void process_command_line(int argc, char **argv);
  void usage();
};

// clang-format off
void MultiOpts::usage()
{
  fprintf(stdout,
"\n"
"Usage: %s [options]\n"
"\n"
"Split audio input on standard input into multiple FIFOs in /tmp. If a FIFO\n"
"cannot be written to, e.g. because it is not being read and its buffer is\n"
"full, the data to be written will be discarded\n"
"\n"
"Options\n"
"%s"
"  -c <file>  configuration file (default: %s)\n"
"\n",
  get_program_name().c_str(), help_ver_text, conf_file.c_str());
}
// clang-format on

void MultiOpts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  int c;

  handle_long_opts(argc, argv);

  while ((c=getopt(argc, argv, ":hc:")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {
    case 'c':
      conf_file = optarg;
      break;

    default:
      error("unknown command line error");
    }
  }
}

void cleanup(void)
{
  for(size_t i=0; i<fifo_paths.size(); i++)
    unlink(fifo_paths[i].c_str());
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


class Fifo
{
public:
  string path;
  int fd_read;
  FILE *fstrm_write;

  Fifo(string fname);
  bool open();
  void close();
};

Fifo::Fifo(string path): path(path), fd_read(-1), fstrm_write(0) {}

bool Fifo::open()
{
  close();
  fd_read = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd_read >= 0) {
     int fd_write = ::open(path.c_str(), O_WRONLY | O_NONBLOCK);
     if (fd_write >= 0)
       fstrm_write = fdopen(fd_write, "w");
  }

  if (fstrm_write==0)
    close();

  return (fstrm_write != 0);
}

void Fifo::close()
{
  if (fstrm_write)
    fclose(fstrm_write);
  fstrm_write = 0;
  
  if (fd_read >= 0)
    ::close(fd_read);
  fd_read = -1;
}


int start_idle_loop(vector<Fifo> &fifos)
{
  const size_t buff_sz = 1024;
  unsigned char buff[buff_sz];
  struct timespec req = { .tv_sec = 0, .tv_nsec = 10000000 }; // 10 ms
  //syslog(3, "Start idle loop");
  while (true) {
    int num_read = fread(buff, sizeof(unsigned char), buff_sz, stdin);
    //syslog(3, "Finished read");
    if(num_read > 0) {
      //syslog(3, "Something to write");
      for (size_t i=0; i<fifos.size(); i++) {
        if (!fwrite(buff, sizeof(unsigned char), num_read,
              fifos[i].fstrm_write)) {
          fifos[i].open();
          //syslog(3, "Closing stream %d", i);
        }
      }
    }
    else
      exit(EXIT_SUCCESS);   // Input closed

    if (num_read < (int)buff_sz)
       nanosleep(&req, NULL);

  }

  return 0;
}

int main(int argc, char **argv)
{
  MultiOpts opts;
  opts.process_command_line(argc, argv);

  vector<string> fifo_names;
  fifo_names.push_back("test2");
  fifo_names.push_back("test3");

  // init_signals();
  // atexit(cleanup);

  string fifo_dir_path = "/tmp/audio_fifos/";
  errno = 0;
  if (mkdir(fifo_dir_path.c_str(), 0777) == -1 && errno != EEXIST) {
    opts.error("could not create FIFO director " + fifo_dir_path + ": " +
               string(strerror(errno)));
  }
  chmod(fifo_dir_path.c_str(), 0777); // make it writable if newly created
  
  vector<Fifo> fifos;
  for (size_t i=0; i<fifo_names.size(); i++) {
    const char *illegal_chars = "\\/:?\"<>|";
    if (strpbrk(fifo_names[i].c_str(), illegal_chars))
      opts.error("FIFO name " + fifo_names[i] +
                 "contains an illegal character from: " + illegal_chars);
    string fifo_name = fifo_dir_path + fifo_names[i];
    fifo_paths.push_back(fifo_name);
    if (mkfifo(fifo_name.c_str(), 0666) == -1 && errno != EEXIST)
      opts.error("could not create FIFO " + fifo_name +
                 " : " + string(strerror(errno)));
    chmod(fifo_name.c_str(), 0666); // make it writable if newly created

    // need a reader so that FIFO writer does not block
    Fifo fifo(fifo_name);
    if (!fifo.open())
      opts.error("could not open FIFO " + fifo.path +
                 " for writing: " + string(strerror(errno)));
    fifos.push_back(fifo);
  }

  int loop_ret = start_idle_loop(fifos);

  if(loop_ret != 0)
    exit(EXIT_FAILURE);

  return 0;
}

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

#include <string>
#include <mpd/client.h>

class mpd_info
{
private:
  int volume;
  std::string origin;
  std::string title;
  int song_elapsed_secs;
  int song_total_secs;
  int kbitrate;
  enum mpd_state state;

  void init_vals();
  void set_vals(struct mpd_connection *conn);
  
public:
  mpd_info();                         // Constructor
  int init();                         // Initialise with current status values
  void print_vals() const;
  
  int get_volume() const;             // Volume: 0 - 100
  std::string get_origin() const;     // Song origin: station, artist, album...
  std::string get_title() const;      // Song title
  int get_elapsed_secs() const;       // Elapsed time of song in seconds
  int get_total_secs() const;         // Total time of song in seconds
  int get_kbitrate() const;           // KBitrate
  std::string get_kbitrate_str() const; // KBitrate as string


  std::string get_elapsed_time() const;  // Elapsed time of song: hh:mm:ss
  std::string get_total_time() const; // Total time of song: hh:mm:ss
  float get_progress() const;         // Progress through song: 0.00 - 1.00
  
  enum mpd_state get_state() const;   // MPD_STATE_: UNKNOWN, STOP, PAUSE, START
};


// Get network connection value
//   -1  :  no interface, or error
//   0-99:  ethernet connection, ethN, value = N
//   100+:  wireless connection, link value + 100
int get_connection_info();


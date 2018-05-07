#include "status.h"
#include "iconv_wrap.h"

#include <mpd/client.h>

#ifdef VOLUMIO
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#endif // VOLUMIO

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <string>

using std::string;

namespace {
  const iconvpp::converter conv("ASCII//TRANSLIT", "UTF-8", true);
}

bool eth_connection(char *ifname)
{
   struct ifreq ifr;

   memset( &ifr, 0, sizeof(ifr) );
   strcpy( ifr.ifr_name, ifname );

   int found = false;
   int dummy_fd = socket( AF_INET, SOCK_DGRAM, 0 );
   if (ioctl(dummy_fd, SIOCGIFFLAGS, &ifr) != -1)
     found = (ifr.ifr_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING);
   close(dummy_fd);
   return found;
}

// Get network connection value
// -1: no interface, or error
// 0-99: ethernet connection, ethN, value = N
// 100+: wireless connection, link value + 100
int get_connection_info()
{
  const char *digits = "0123456789";
  char ifname[] = "ethN";
  for(int i=0; i<4; i++) {
    ifname[3] = digits[i];
    if(eth_connection(ifname))
      return i;
  }

  FILE *fproc = fopen("/proc/net/wireless", "r");

  if (fproc == NULL) {
    perror("failed to open /proc/net/wireless");
    return -1;
  }

  char buf[128];
  for (int i = 0; i < 30; i++) { // Link is in position 30
    if (fscanf(fproc, "%s", buf) == EOF) {
      strcpy(buf, "0");
      break;
    }
  }

  fclose(fproc);

  return 100 + atoi(buf);
}

#ifdef VOLUMIO
// https://stackoverflow.com/questions/24884490/using-libcurl-and-jsoncpp-to-parse-from-https-webserver
namespace
{
    std::size_t callback(
            const char* in,
            std::size_t size,
            std::size_t num,
            std::string* out)
    {
        const std::size_t totalBytes(size * num);
        out->append(in, totalBytes);
        return totalBytes;
    }
}

string get_volumio_status()
{
  const std::string url("http://localhost:3000/api/v1/getstate");

  CURL* curl = curl_easy_init();

  // Set remote URL.
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  // Don't bother trying IPv6, which would increase DNS resolution time.
  curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

  // Time out after 1 second.
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);

  // Follow HTTP redirects if necessary.
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  // Hook up data handling function.
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

  // Hook up data container (will be passed as the last parameter to the
  // callback handling function).  Can be any pointer type, since it will
  // internally be passed as a void pointer.
  string httpData; 
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpData);

  // Run our HTTP GET command, capture the HTTP response code, and clean up.
  curl_easy_perform(curl);
  int httpCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
  curl_easy_cleanup(curl);
  
  return (httpCode == 200) ? httpData : string();
}
#endif // VOLUMIO

static string get_tag(const struct mpd_song *song, enum mpd_tag_type type)
{
  string tag_vals;
  const char *value;
  unsigned i = 0;
  while ((value = mpd_song_get_tag(song, type, i++)) != NULL) {
    if (i > 1)
      tag_vals += "; ";
    tag_vals += value;
  }
  string ascii;
  conv.convert(tag_vals, ascii);
  return ascii;
}
  
mpd_info::mpd_info()
{
  init_vals();
}

void mpd_info::init_vals()
{
  state = MPD_STATE_UNKNOWN;
  volume = 0;
  origin = string();
  title = string();
  song_elapsed_secs = 0;
  song_total_secs = 0;
  kbitrate = 0;
}

void mpd_info::set_vals(struct mpd_connection *conn)
{
  mpd_command_list_begin(conn, true);
  mpd_send_status(conn);
  mpd_send_current_song(conn);
  mpd_command_list_end(conn);

  struct mpd_status *status = mpd_recv_status(conn);
  if (status == NULL)
    return;

#ifndef VOLUMIO // Volumio volume is not MPD volume
  volume = mpd_status_get_volume(status);
  if (mpd_status_get_error(status) != NULL)
    return;
#endif // VOLUMIO

  state = mpd_status_get_state(status);
  if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
    song_elapsed_secs = mpd_status_get_elapsed_time(status);
    song_total_secs = mpd_status_get_total_time(status);
    kbitrate = mpd_status_get_kbit_rate(status);
  }

  mpd_status_free(status);

  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
    return;

  mpd_response_next(conn);

  struct mpd_song *song;
  if ((song = mpd_recv_song(conn)) != NULL) {
    title = get_tag(song, MPD_TAG_TITLE);

    // Where does the song come from, just one choice
    enum mpd_tag_type origin_tags[] = {
      MPD_TAG_ARTIST, MPD_TAG_NAME, MPD_TAG_ALBUM_ARTIST,
      MPD_TAG_COMPOSER, MPD_TAG_PERFORMER, MPD_TAG_UNKNOWN };
    int i = 0;
    while (origin_tags[i] != MPD_TAG_UNKNOWN) {
      origin = get_tag(song, origin_tags[i]);
      if (origin.size())
        break;
      i++;
    }

    mpd_song_free(song);
  }

  mpd_response_finish(conn);
}

void mpd_info::print_vals() const
{
  fprintf(stdout, "state: %d\n", (int)state);
  fprintf(stdout, "volume: %d\n", volume);
  fprintf(stdout, "origin: %s\n", origin.c_str());
  fprintf(stdout, "title: %s\n", title.c_str());
  fprintf(stdout, "song_elapsed_secs: %d\n", song_elapsed_secs);
  fprintf(stdout, "song_total_secs: %d\n", song_total_secs);
  fprintf(stdout, "song_progress: %d\n",  int(100*get_progress()));
  fprintf(stdout, "kbitrate: %d\n", kbitrate);
}

int mpd_info::init()
{
  struct mpd_connection *conn = mpd_connection_new(NULL, 0, 30000);
  if (mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS)
    set_vals(conn);
  int ret = (mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS);
  mpd_connection_free(conn);

#ifdef VOLUMIO
  string volumio_status = get_volumio_status();
  Json::Reader reader;
  Json::Value obj;
  if (reader.parse(volumio_status, obj)) {
    volume = obj["volume"].isInt() ? obj["volume"].asInt() : 0;
  }
  else {
    volume = 0;
  }
#endif // VOLUMIO

  return ret;
}

static string secs_to_time(int secs)
{
  int hours = secs / 3600;
  secs = secs % 3600;
  int mins = secs / 60;
  secs %= 60;

  if (hours > 99)
    hours = 99;

  const size_t time_str_len = 9;
  char time_str[time_str_len];
  snprintf(time_str, time_str_len, "%02d:%02d:%02d", hours, mins, secs);
  return time_str;
} 

string mpd_info::get_elapsed_time() const
{
  return secs_to_time(song_elapsed_secs);
}

string mpd_info::get_total_time() const
{
  return secs_to_time(song_total_secs);
}

float mpd_info::get_progress() const
{
  return song_total_secs ? (float)song_elapsed_secs/song_total_secs : 0.0;
}

string mpd_info::get_kbitrate_str() const
{
  int rate = std::min(kbitrate, 999);
  const size_t str_len = 5;
  char str[str_len];
  snprintf(str, str_len, "%4d", rate);
  return str;
} 


int mpd_info::get_volume() const { return volume; }

string mpd_info::get_origin() const { return origin; }

string mpd_info::get_title() const { return title; }

int mpd_info::get_elapsed_secs() const { return song_elapsed_secs; }

int mpd_info::get_total_secs() const { return song_total_secs; }

int mpd_info::get_kbitrate() const { return kbitrate; }

enum mpd_state mpd_info::get_state() const { return state; }



/*
   Copyright (c) 2003-2018, Adrian Rossiter

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

/*!\file programopts.cc
   \brief program option handling
*/

#include "programopts.h"
#include "utils.h"
#include <map>
#include <string.h>

using std::map;
using std::string;

const int MSG_SZ = 256;

const char *ProgramOpts::help_ver_text = "  -h,--help this help message\n"
                                         "  --version version information\n";

string ProgramOpts::get_program_name() const { return program_name; }
string ProgramOpts::get_version_no() const { return version_no; }

void ProgramOpts::message(string msg, const char *msg_type, string opt) const
{
  fprintf(stderr, "%s: ", get_program_name().c_str());
  if (msg_type)
    fprintf(stderr, "%s: ", msg_type);
  if (opt != "") {
    if (opt.size() == 1 || opt[0] == '\0')
      fprintf(stderr, "option -%s: ", opt.c_str());
    else
      fprintf(stderr, "%s: ", opt.c_str());
  }

  fprintf(stderr, "%s\n", msg.c_str());
}

void ProgramOpts::error(std::string msg, std::string opt, int exit_num) const
{
  message(msg, "error", opt);
  exit(exit_num);
}

void ProgramOpts::error(std::string msg, char opt, int exit_num) const
{
  message(msg, "error", std::string() + opt);
  exit(exit_num);
}

void ProgramOpts::warning(std::string msg, std::string opt) const
{
  message(msg, "warning", opt);
}

void ProgramOpts::warning(std::string msg, char opt) const
{
  message(msg, "warning", std::string() + opt);
}

void ProgramOpts::print_status_or_exit(const Status &stat,
                                       std::string opt) const
{
  if (stat.is_warning())
    warning(stat.msg(), opt);
  else if (stat.is_error())
    error(stat.msg(), opt);
  else if (stat.is_ok() && stat.msg() != "") // only print if there is a message
    message(stat.msg(), 0, opt);
}

void ProgramOpts::print_status_or_exit(const Status &stat, char opt) const
{
  print_status_or_exit(stat, std::string() + opt);
}

bool ProgramOpts::common_opts(char c, char opt)
{
  switch (c) {
  case 'h':
    usage();
    exit(0);

  case '?':
    error("unknown option", string("-") + opt);
    break;

  case ':':
    error("missing argument", string("-") + opt);
    break;

  default:
    return false;
  }
  return true;
}

void ProgramOpts::handle_long_opts(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      usage();
      exit(0);
    }
    else if (strcmp(argv[i], "--version") == 0) {
      version();
      exit(0);
    }
    else if (strncmp(argv[i], "--", 2) == 0 && strlen(argv[i]) > 2)
      error("unknown option", argv[i]);
  }
}

static string itostr(int i)
{
  char buf[MSG_SZ];
  snprintf(buf, MSG_SZ - 1, "%d", i);
  return buf;
}

Status ProgramOpts::get_arg_id(const char *arg, string *arg_id,
                               const char *maps, unsigned int match_flags)
{
  *arg_id = ""; // invalid value
  char arg_lower[MSG_SZ];
  bool ignore_case = !(argmatch_case_sensitive & match_flags);
  if (ignore_case) { // make lowercase copy of arg
    char *q = arg_lower;
    for (const char *p = arg; *p; ++p)
      *q++ = tolower(*p);
    *q = '\0';
  }
  const char *argu = (ignore_case) ? arg_lower : arg;

  // set up arg -> id map
  map<string, string> mps;
  string argument, id;
  int pos_no = 0;
  int in_arg = true;
  const char *p = maps;
  while (true) {
    if (*p == '|' || *p == '\0') { // end of argument, id pair
      if (id == "")                // no '=' in pair so ID is position number
        id = itostr(pos_no);
      if (argument != "") // don't stor empty pairs
        mps[argument] = id;
      if (!*p) // end of string, finish processing
        break;
      argument.clear(); // start of new argument, id pair
      id.clear();
      in_arg = true;
      pos_no++;
    }
    else if (*p == '=' && in_arg) // end of arg, consider id next
      in_arg = false;
    else {
      if (in_arg) {
        // store argument as lowercase if 'no_case' compare
        char q = (argmatch_case_sensitive & match_flags) ? *p : tolower(*p);
        argument += q;
      }
      else
        id += *p;
    }

    p++;
  }

  map<string, string>::iterator mi;
  if (argmatch_add_id_maps & match_flags) {
    for (mi = mps.begin(); mi != mps.end(); ++mi)
      if (mps.find(mi->second) == mps.end())
        mps[mi->second] = mi->second;
  }

  // look for match
  mi = mps.lower_bound(argu);

  if (mi != mps.end() && mi->first == argu) { // exact match
    *arg_id = mi->second;
    return Status::ok();
  }

  // look for valid partial match
  if (!(argmatch_no_partial & match_flags) && // partial matches allowed
      mi != mps.end() && mi->first.substr(0, strlen(argu)) == argu) {
    // is partial match, check for ambiguity
    map<string, string>::iterator mi2 = mi;
    bool ambiguous = false;
    // run through subsequent partial matches
    while (++mi2 != mps.end() && mi2->first.substr(0, strlen(argu)) == argu)
      if (mi2->second != mi->second) { // different values, ambiguous
        ambiguous = true;
        break;
      }

    if (!ambiguous) { // valid partial match
      *arg_id = mi->second;
      return Status::ok();
    }
    else
      return Status::error(msg_str("ambiguous argument '%s'", arg));
  }

  // no valid match found
  return Status::error(msg_str("invalid argument '%s'", arg));
}

void ProgramOpts::version()
{
  fprintf(stdout, "%s %s: https://github.com/antiprism/mpd_oled\n",
          get_program_name().c_str(), get_version_no().c_str());
}

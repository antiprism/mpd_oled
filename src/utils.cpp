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

/* \file utils.cpp
   \brief utility routines for maths operations, text operations,
   I/O conversions, etc
*/
#include "utils.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

using std::string;
using std::vector;

/// Whitespace characters
const char WHITESPACE[] = " \t\r\n\f\v";

Status read_double(const char *str, double *f)
{
  bool to_sqrt;
  char buff;
  if (sscanf(str, " sqrt%lf %c", f, &buff) == 1)
    to_sqrt = true;
  else if (sscanf(str, " %lf %c", f, &buff) == 1)
    to_sqrt = false;
  else
    return Status::error("not a number");

  if (isinf(*f))
    return Status::error("number too large\n");

  if (isnan(*f))
    return Status::error("not a number\n");

  if (to_sqrt)
    *f = sqrt(*f);

  return Status::ok();
}

Status read_int(const char *str, int *i)
{
  char buff;
  if (sscanf(str, " %d %c", i, &buff) != 1)
    return Status::error("not an integer");

  if (*i == INT_MAX)
    return Status::error("integer too large\n");

  return Status::ok();
}

Status read_int_list(vector<char *> &vals, vector<int> &nums, bool is_index)
{
  nums.clear();
  int num;
  for (size_t i = 0; i < vals.size(); i++) {
    if (!read_int(vals[i], &num))
      return Status::error(msg_str("'%s' is not an integer", vals[i]));

    if (is_index && num < 0)
      return Status::error(msg_str("'%s' is not a positive integer", vals[i]));

    nums.push_back(num);
  }

  return Status::ok();
}

Status read_int_list(char *str, vector<int> &nums, bool is_index, int len,
                     const char *sep)
{
  nums.clear();
  int vec_idx;
  char *v_str = strtok(str, sep);
  int i = 0;
  while (v_str) {
    i++;
    if (!read_int(v_str, &vec_idx))
      return Status::error(msg_str("'%s' is not an integer", v_str));

    if (is_index && vec_idx < 0)
      return Status::error(msg_str("'%s' is not a positive integer", v_str));

    if (len && i > len)
      return Status::error(msg_str("more than %d integers given", len));

    nums.push_back(vec_idx);
    v_str = strtok(0, sep);
  }

  return Status::ok();
}

Status read_double_list(vector<char *> &vals, vector<double> &nums)
{
  nums.clear();
  double num;
  for (size_t i = 0; i < vals.size(); i++) {
    Status stat = read_double(vals[i], &num);
    if (stat.is_error())
      return Status::error(msg_str("%s: '%s'", stat.c_msg(), vals[i]));

    nums.push_back(num);
  }
  return Status::ok();
}

Status read_double_list(char *str, vector<double> &nums, int len,
                        const char *sep)
{
  nums.clear();
  double num;
  char *num_str = strtok(str, sep);
  int i = 0;
  while (num_str) {
    i++;
    Status stat = read_double(num_str, &num);
    if (stat.is_error())
      return Status::error(msg_str("%s: '%s'", stat.c_msg(), num_str));

    if (len && i > len)
      return Status::error(msg_str("more than %d numbers given", len));

    nums.push_back(num);
    num_str = strtok(0, sep);
  }

  return Status::ok();
}

int split_line(char *line, vector<char *> &parts, const char *delims,
               bool strict)
{
  parts.clear();
  if (!delims)
    delims = WHITESPACE;

  if (strict) {
    char *cur = line;
    parts.push_back(cur);                    // always an entry, even if null
    while (*(cur += strcspn(cur, delims))) { // quit at end of string
      *cur = '\0';                           // terminate part
      cur++;                                 // start of next part
      parts.push_back(cur);                  // add even if final null
    }
    /*while(*cur) {                             // quit at end of string
       cur += strcspn(cur, delims);           // next delimiter
       if(*cur) {                             // part ended with delimiter
          *cur = '\0';                        // terminate part
          cur++;                              // start of next part
          parts.push_back(cur);                // add even if final null
       }
    }*/
  }
  else {
    char *val;
    if (!(val = strtok(line, delims)))
      return 0;

    parts.push_back(val);
    while ((val = strtok(0, delims)))
      parts.push_back(val);
  }

  return parts.size();
}

string msg_str(const char *fmt, ...)
{
  const int MSG_SZ = 256;
  char message[MSG_SZ];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, MSG_SZ - 1, fmt, args);
  return message;
}

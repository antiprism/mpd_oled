/*
   Copyright (c) 2003-2016, Adrian Rossiter

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

/**\file timer.cc
   \brief Timing utilities
*/

#include "timer.h"
#include <unistd.h>

long to_long_usecs(timeval tv)
{
  return tv.tv_sec * 1000000 + tv.tv_usec;
}


timeval &tv_normalise(timeval &tv)
{
  long sec = tv.tv_usec / 1000000;
  tv.tv_usec -= sec * 1000000;
  if (tv.tv_usec < 0) {
    tv.tv_usec += 1000000;
    tv.tv_sec -= 1;
  }
  tv.tv_sec += sec;
  return tv;
}


bool operator>(const timeval &t0, const timeval &t1)
{
  return (t0.tv_sec > t1.tv_sec ||
          (t0.tv_sec == t1.tv_sec && t0.tv_usec > t1.tv_usec));
}

timeval operator+(const timeval &t0, const timeval &t1)
{
  timeval ret;
  ret.tv_sec = t0.tv_sec + t1.tv_sec;
  ret.tv_usec = t0.tv_usec + t1.tv_usec;
  tv_normalise(ret);
  return ret;
}


timeval operator-(const timeval &t0, const timeval &t1)
{
  timeval ret;
  ret.tv_sec = t0.tv_sec - t1.tv_sec;
  ret.tv_usec = t0.tv_usec - t1.tv_usec;
  return tv_normalise(ret);
}

timeval to_timeval(double tm)
{
  timeval tv;
  tv.tv_sec = long(tm);
  tv.tv_usec = long((tm - tv.tv_sec) * 1000000);
  return tv;
}

void Timer::set_timer(timeval interval)
{
  timeval tv;
  gettimeofday(&tv, 0);
  end = tv + interval;
}

void Timer::set_timer(double interval)
{
  set_timer(to_timeval(interval));
}

void Timer::inc_timer(double inc)
{
  end = end + to_timeval(inc);
}

bool Timer::finished()
{
  timeval tv;
  gettimeofday(&tv, 0);
  return tv > end;
}

void Timer::sleep_until_finished()
{
  timeval tv;
  gettimeofday(&tv, 0);
  if (end > tv)
    sleep(to_long_usecs(end - tv));
}




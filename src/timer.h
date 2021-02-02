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

/**\file timer.h
   \brief Timing utilities
*/

#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

/// A subsecond %Timer
class Timer {
private:
  timeval end;

public:
  /// Constructor
  Timer(double interval = 0.0) { set_timer(interval); }

  /// Set the %Timer.
  /**\param interval length of time the %Timer should run. */
  void set_timer(timeval interval);

  /// Set the %Timer.
  /**\param interval length of time in microseconds that the %Timer
   * should run. */
  void set_timer(double interval);

  /// Increment the %Timer.
  /**\param inc length of time in microseconds that the %Timer
   * should be extended. */
  void inc_timer(double inc);

  /// Check whether the %timer has finished.
  /**\return \c true if the %timer has finished, otherwise \c false. */
  bool finished();

  /// Sleep until finished.
  /**Pause program execution for the amount of time remaining
   * on the %timer. */
  void sleep_until_finished();
};

/// A subsecond %Counter
class Counter {
private:
  timeval start;

  // int get_time(struct timeval *tp); // wrapper for subsecond current time

public:
  /// Constructor
  Counter() { reset(); }

  /// Reset and start the %Counter.
  void reset();

  /// Get the number of usecs since the %Counter started
  /**\return The number of usecs since the Counter started */
  long usecs() const;

  /// Get the number of secs since the %Counter started
  /**\return The number of secs since the Counter started */
  double secs() const;
};

#endif // TIMER_H

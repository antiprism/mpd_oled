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

/*!\file utils.h
   \brief utility routines for text operations, I/O conversions, etc
*/

#ifndef UTILS_H
#define UTILS_H

#include "status_msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

/// Read a floating point number, or mathematical expression, from a string.
/** The string should only hold the floating point number or expression, but may
 *  have leading and trailing whitespace.
 * \param str the string holding the floating point number or expressions.
 * \param f used to return the floating point number.
 * \return status, evaluates to \c true if a valid floating point number
 *  was read, otherwise \c false.*/
Status read_double(const char *str, double *f);

/// Read an integer from a string.
/** The string should only hold the integer, but may
 *  have leading and trailing whitespace.
 * \param str the string holding the integer.
 * \param i used to return the integer.
 * \return status, evaluates to \c true if a valid integer
 *  was read, otherwise \c false.*/
Status read_int(const char *str, int *i);

/// Read floating point numbers, or mathematical expressions, from a list of
/// strings.
/** The strings should only hold the floating point number, but may
 *  have leading and trailing whitespace.
 * \param vals the strings holding the floating point numbers.
 * \param nums used to return the floating point numbers.
 * \return status, evaluates to \c true if only valid floating point numbers
 *  were read, otherwise \c false.*/
Status read_double_list(std::vector<char *> &vals, std::vector<double> &nums);

/// Read floating point numbers listed in a single string.
/** The numbers in the string should be comma separated, and may
 *  have leading and trailing whitespace.
 * \param str the string holding the comma-separated floating point numbers.
 * \param nums used to return the floating point numbers.
 * \param len the maximum total of numbers that should be in \a str, or if
 *  it is \c 0 then there is no maximum.
 * \param sep the characters that can separate the numbers.
 * \return status, evaluates to \c true if only valid floating point numbers
 *  were read, , and no more than \a len (if \c len>0),otherwise \c false.*/
Status read_double_list(char *str, std::vector<double> &nums, int len = 0,
                        const char *sep = ",");

/// Read integers from a list of strings.
/** The strings should only hold the integers, but may
 *  have leading and trailing whitespace.
 * \param vals the strings holding the integers.
 * \param nums used to return the integers.
 * \param is_index if true then the integers cannot be negative.
 * \return status, evaluates to \c true if only valid integers
 *  were read, otherwise \c false.*/
Status read_int_list(std::vector<char *> &vals, std::vector<int> &nums,
                     bool is_index = false);

/// Read integers listed in a single string.
/** The numbers in the string should be comma separated, and may
 *  have leading and trailing whitespace.
 * \param str the string holding the comma-separated integers.
 * \param nums used to return the integers.
 * \param is_index if true then the integers cannot be negative.
 * \param len the maximum total of numbers that should be in \a str, or if
 *  it is \c 0 then there is no maximum.
 * \param sep the characters that can separate the numbers.
 * \return status, evaluates to \c true if only valid integers
 *  were read, , and no more than \a len (if \c len>0),otherwise \c false.*/
Status read_int_list(char *str, std::vector<int> &nums, bool is_index = false,
                     int len = 0, const char *sep = ",");

/// Split a line into delimited parts
/**\param line the line to split (this will be modified).
 * \param parts the parts of the split line.
 * \param delims the characters to use as delimiters, if \c 0 then use
 *  whitespace characters.
 * \param strict if true then treat every delimiter as a separator, returning
 *  null strings between adjacent delimiters, always returning at least
 *  one part.
 * \return The number of parts. */
int split_line(char *line, std::vector<char *> &parts, const char *delims = 0,
               bool strict = false);

/// Join strings into a single string, with parts separated by a delimiter
/**\param iterator to first string in sequence
 * \param iterator to end of string sequence
 * \param delim delimiter to include between strings
 * \return The joined and delimited strings */
template <typename InputIt>
std::string join(InputIt first, InputIt last, std::string delim = " ")
{
  std::string str;
  for (; first != last; ++first)
    str += *first + ((std::next(first) != last) ? delim : std::string());
  return str;
}

/// Remove leading and trailing space, convert any whitespace to a single space
/**\param str the string to convert.
 * \return A pointer to the string. */
char *clear_extra_whitespace(char *str);

/// Convert a C formated message string to a C++ string
/** Converts the first MSG_SZ-1 characters of the C format string
 * \param fmt the formatted string
 * \param ... the values for the format
 * \return The converted string. */
std::string msg_str(const char *fmt, ...);

#endif // UTILS_H

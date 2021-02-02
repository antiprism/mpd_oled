/*
   Copyright (c) 2014-2018, Adrian Rossiter

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

/*!\file status_msg.h
   \brief class to handle return status
*/

#ifndef STATUS_MSG_H
#define STATUS_MSG_H

#include <string>

/// Status
class Status {
private:
  enum { STATUS_OK = 0, STATUS_WARNING = 1 << 3, STATUS_ERROR = 1 << 10 };
  int status_code;
  std::string message;
  void set_status_message(const std::string &msg, int code, int type);

public:
  /// Default constructor
  Status() : status_code(STATUS_OK) {}

  /// Check if status is ok
  /**\return true if the status is normal, otherwise false.*/
  operator bool() const;

  /// Check if status is ok
  /**\return true if the status is normal, otherwise false.*/
  bool is_ok() const;

  /// Check if status is warning
  /**\return true if the status is normal, otherwise false.*/
  bool is_warning() const;

  /// Check if status is error
  /**\return true if the status is normal, otherwise false.*/
  bool is_error() const;

  /// Set status to ok, with message and type
  /**\param msg status message
   * \param code low number indicating type of ok status
   * \return reference to this status.*/
  Status &set_ok(const std::string &msg = std::string(), int code = 0);

  /// Set status to warning, with message and type
  /**\param msg status message
   * \param code low number indicating type of warning status
   * \return reference to this status.*/
  Status &set_warning(const std::string &msg, int code = 0);

  /// Set status to error, with message and type
  /**\param msg status message
   * \param code low number indicating type of error status
   * \return reference to this status.*/
  Status &set_error(const std::string &msg, int code = 0);

  /// Get status message
  /**\return The status message.*/
  const std::string &msg() const;

  /// Get status message as C string
  /**\return The status message.*/
  const char *c_msg() const;

  /// Get status code
  /**The code is a low number, and particular to the status type
   * \return The status code.*/
  int code() const;

  /// Get an ok status, with message and type
  /**\param msg status message
   * \param code low number indicating type of ok status.
   * \return am ok status.*/
  static Status ok(const std::string &msg = std::string(), int code = 0);

  /// Get a warning status, with message and type
  /**\param msg status message
   * \param code low number indicating type of warning status.
   * \return a warning status.*/
  static Status warning(const std::string &msg, int code = 0);

  /// Get an error status, with message and type
  /**\param msg status message
   * \param code low number indicating type of error status.
   * \return an error status.*/
  static Status error(const std::string &msg, int code = 0);
};

inline void Status::set_status_message(const std::string &msg, int code,
                                       int type)
{
  message = msg;
  status_code = code + type;
}

inline Status::operator bool() const { return is_ok(); }

inline bool Status::is_ok() const
{
  return status_code >= STATUS_OK && status_code < STATUS_WARNING;
}

inline bool Status::is_warning() const
{
  return status_code >= STATUS_WARNING && status_code < STATUS_ERROR;
}

inline bool Status::is_error() const { return status_code >= STATUS_ERROR; }

inline Status &Status::set_ok(const std::string &msg, int code)
{
  set_status_message(msg, code, STATUS_OK);
  return *this;
}

inline Status &Status::set_warning(const std::string &msg, int code)
{
  set_status_message(msg, code, STATUS_WARNING);
  return *this;
}

inline Status &Status::set_error(const std::string &msg, int code)
{
  set_status_message(msg, code, STATUS_ERROR);
  return *this;
}

inline const std::string &Status::msg() const { return message; }

inline const char *Status::c_msg() const { return message.c_str(); }

inline int Status::code() const
{
  int ret = status_code;
  if (is_warning())
    ret += STATUS_WARNING;
  else if (is_error())
    ret += STATUS_ERROR;
  return ret;
}

#endif // STATUS_MSG_H

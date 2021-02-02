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

/*!\file programopts.h
   \brief program option handling
*/

#ifndef PROGRAMOPTS_H
#define PROGRAMOPTS_H

#include "status_msg.h"
#include "ultragetopt.h"
#include <string>

/// Command line processing
class ProgramOpts : public GetOpt {
private:
  std::string program_name;
  std::string version_no;

public:
  enum {
    argmatch_default = 0,
    argmatch_case_sensitive = 1,
    argmatch_no_partial = 2,
    argmatch_add_id_maps = 4
  };

  static const char *help_ver_text;

  /// Constructor
  /**\param prog_name the name of the program. */
  ProgramOpts(std::string prog_name, std::string vers_no = "")
      : program_name(prog_name), version_no(vers_no)
  {
  }

  /// Destructor
  virtual ~ProgramOpts() {}

  /// Process the command line
  /** In the derived class this will process the program options
   *  and arguments, probably using \c getopt. */
  virtual void process_command_line(int /*argc*/, char ** /*argv*/){};

  /// Usage message
  /** In the derived class this will print a program usage help message*/
  virtual void usage(){};

  /// Usage message
  /** print a version message*/
  void version();

  /// Get the program name
  /**\return the program name. */
  std::string get_program_name() const;

  /// Get the version number
  /**\return the version number. */
  std::string get_version_no() const;

  /// Print a message (to standard error).
  /** The message will be preceded by the program name, the
   *  message type (if given), and the option letter or
   *  argument name (if given).
   * \param msg the message to print.
   * \param msg_type the message type (e.g. 'warning').
   * \param opt the option letter or argument name. */
  void message(std::string msg, const char *msg_type = 0,
               std::string opt = "") const;

  /// Print an error message (to standard error) and exit.
  /** The message will be preceded by the program name, and the
   *  option letter or argument name (if given).
   * \param msg the message to print.
   * \param opt the option letter or argument name.
   * \param exit_num The value to return when the program exits. */
  void error(std::string msg, std::string opt = "", int exit_num = 1) const;

  /// Print an error message (to standard error) and exit.
  /** The message will be preceded by the program name, and the
   *  option letter (if given).
   * \param msg the message to print.
   * \param opt the option letter.
   * \param exit_num The value to return when the program exits. */
  void error(std::string msg, char opt, int exit_num = 1) const;

  /// Print a warning message (to standard error).
  /** The message will be preceded by the program name, and the
   *  option letter or argument name (if given).
   * \param msg the message to print.
   * \param opt the option letter or argument name. */
  void warning(std::string msg, std::string opt = "") const;

  /// Print a warning message (to standard error).
  /** The message will be preceded by the program name, and the
   *  option letter (if given).
   * \param msg the message to print.
   * \param opt the option letter. */
  void warning(std::string msg, char opt) const;

  /// Print a status message, and exit if error (to standard error).
  /** The message will be preceded by the program name, and the
   *  option letter (if given).
   * \param stat the status message to print.
   * \param opt the option letter. */
  void print_status_or_exit(const Status &stat, std::string opt = "") const;

  /// Print a status message, and exit if error (to standard error).
  /** The message will be preceded by the program name, and the
   *  option letter (if given).
   * \param stat the status message to print.
   * \param opt the option letter. */
  void print_status_or_exit(const Status &stat, char opt) const;

  /// Process long options
  /**\param argc the number of arguments.
   * \param argv pointers to the argument strings. */
  void handle_long_opts(int argc, char *argv[]);

  /// Process common options
  /**\param c the character returned by getopt.
   * \param opt the option character being considered by getopt.
   * \return whether the option was handled. */
  bool common_opts(char c, char opt);

  /// Map option arguments to identifiers using matching
  /**\param arg the option argument
   * \param arg_id identifier corresponding to the matched string, or
   *  "" if the argument is not matched.
   * \param maps a set of maps from argument strings to identifiers
   *  separated by '|', e.g. 'string1=id1|sting2=id2|string3=id3'
   * \param match_flags the default is a icase insesetive match of
   *  \a arg to a string or failing that to the start of exactly one string.
   *  \c argmatch_case_sensitive distinguishes case, \c argmatch_no_partial
   *  disallows partial matches, \c argmatch_add_id_maps add extra maps
   *  so each identifiers maps to itself.
   * \return status, evaluates to \c true if a valid identifier was found,
   *  otherwise \c false.*/
  Status get_arg_id(const char *arg, std::string *arg_id, const char *maps,
                    unsigned int match_flags = argmatch_default);
};

#endif // PROGRAMOPTS_H

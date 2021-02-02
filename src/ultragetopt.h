/* Ultra-Getopt - A replacement for getopt() with support for many common
 *		  extensions, MS-DOS formatted option strings, and much more.
 *
 * To use this library as a replacement for vendor-provided getopt() functions,
 * define ULTRAGETOPT_REPLACE_GETOPT and include "ultragetopt.h" after the
 * vendor-provided headers for getopt() functions.
 *
 * Copyright (c) 2007, Kevin Locke <kwl7@cornell.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* Adrian Rossiter: converted to a class for ANTIPRISM */

/*!\file getopt.h
 * \brief Ultragetopt: replacement for getopt
 */

/* define ULTRAGETOPT_REPLACE_GETOPT for ultragetopt*() to replace getopt*() */
/* define ULTRAGETOPT_ONLY_DOS for ultragetopt*_dos() to replace ultragetopt*()
 */

#ifndef INCLUDED_GETOPT_H
#define INCLUDED_GETOPT_H 1

// ANTIPRISM: defined here for convenience
#define ULTRAGETOPT_REPLACE_GETOPT
#define ULTRAGETOPT_LIKE_GNU

/* Flag values to pass to getopt_tunable() */
#define UGO_CASEINSENSITIVE 0x1
#define UGO_SINGLELEADERLONG 0x2
#define UGO_OPTIONPERMUTE 0x4
#define UGO_NONOPTARG 0x8
#define UGO_NOPRINTERR 0x10
#define UGO_OPTIONALARG 0x20
#define UGO_MISSINGCOLON 0x40
#define UGO_SEPARATEDOPTIONAL 0x80
#define UGO_SHORTOPTASSIGN 0x100
#define UGO_NOEATDASHDASH 0x200
#define UGO_LONGOPTADJACENT 0x400
#define UGO_HYPHENARG 0x800
#define UGO_SINGLELEADERONLY 0x1000

#ifndef required_argument
#define no_argument 0
#define required_argument 1
#define optional_argument 2

struct option {
  const char *name; /* Name of the option */
  int has_arg;      /* Does the option take an argument? */
  int *flag;        /* Location to store val when option encountered */
  int val;          /* Value to return (or store in flag) for option */
};
#endif /* required_argument */

class GetOpt {
public:
  char *ultraoptarg;
  int ultraoptind;
  int ultraopterr;
  int ultraoptreset;
  int ultraoptopt;

private:
/* Add format error checking for gcc versions that support it */
#if defined(__GNUC__) && __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR > 6)
  // ANTIPRISM: changed (printf, 2, 3) to (printf, 3, 4) for C++
  void print_error(int flags, const char *templat, ...)
      __attribute__((format(printf, 3, 4)));
#else
  void print_error(int flags, const char *templat, ...);
#endif
  int has_separate_argument(int curopt, int argc, char *const argv[],
                            const char *shortopts,
                            const struct option *longopts,
                            const char *assigners, const char *optleaders,
                            int flags);
  int permute_options(int argc, char *argv[], const char *shortopts,
                      const struct option *longopts, const char *assigners,
                      const char *optleaders, int flags);
  int handle_longopt(int longind, char *longarg, int noseparg,
                     char *const argv[], const struct option *longopts,
                     int *indexptr, const char *optleaders, int flags);

public:
  GetOpt();
  int ultragetopt(int argc, char *const argv[], const char *optstring);
  int ultragetopt_long(int argc, char *const argv[], const char *shortopts,
                       const struct option *longopts, int *indexptr);
  int ultragetopt_long_only(int argc, char *const argv[], const char *shortopts,
                            const struct option *longopts, int *indexptr);

  /* Getopt with modifiable (tunable) behavior - also the backend for
   * all other getopt functions.
   * assigners - string of characters accepted to assign to an option
   *          (e.g. --outfile=file.txt where '=' is the assigner)
   * optleaders - string of characters that indicate an option
   *           (usually "-" on UNIX, "/" on DOS)
   * flags - see README for list of accepted flags
   */
  int ultragetopt_tunable(int argc, char *const argv[], const char *shortopts,
                          const struct option *longopts, int *indexptr,
                          const char *assigners, const char *optleaders,
                          int flags);
};

#ifdef ULTRAGETOPT_REPLACE_GETOPT
#define optarg ultraoptarg
#define optind ultraoptind
#define opterr ultraopterr
#define optopt ultraoptopt
#define optreset ultraoptreset
#define getopt(argc, argv, optstring) ultragetopt(argc, argv, optstring)
#define getopt_long(argc, argv, shortopts, longopts, indexptr)                 \
  ultragetopt_long(argc, argv, shortopts, longopts, indexptr)
#define getopt_long_only(argc, argv, shortopts, longopts, indexptr)            \
  ultragetopt_long_only(argc, argv, shortopts, longopts, indexptr)
#define getopt_dos(argc, argv, optstring) ultragetopt_dos(argc, argv, optstring)
#define getopt_long_dos(argc, argv, shortopts, longopts, indexptr)             \
  ultragetopt_long_dos(argc, argv, shortopts, longopts, indexptr)
#endif /* GETOPT_NO_EXTENSIONS */

#ifdef ULTRAGETOPT_DOS_ONLY
#define ultragetopt(argc, argv, optstring)                                     \
  ultragetopt_dos(argc, argv, optstring)
#define ultragetopt_long(argc, argv, shortopts, longopts, indexptr)            \
  ultragetopt_long_dos(argc, argv, shortopts, longopts, indexptr)
#define ultragetopt_long_only(argc, argv, shortopts, longopts, indexptr)       \
  ultragetopt_long_dos(argc, argv, shortopts, longopts, indexptr)
#endif /* ULTRAGETOPT_DOS_ONLY */

#endif /* INCLUDED_GETOPT_H */

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

#include <assert.h>
#include <ctype.h> /* islower() isupper() tolower() toupper() */
#include <stdarg.h>
#include <stdio.h>  /* fprintf() */
#include <stdlib.h> /* getenv() */
#include <string.h> /* strcmp(), strncmp(), strchr() */

#undef ULTRAGETOPT_REPLACE_GETOPT /* Protect against project-wide defines */
#include "ultragetopt.h"

/* Define replacements for missing functions */
#if !HAVE_STRCASECMP && HAVE__STRICMP
#define strcasecmp _stricmp
#endif

#if !HAVE_STRNCASECMP && HAVE__STRNICMP
#define strncasecmp _strnicmp
#endif

#if !HAVE_STRCHR && HAVE_INDEX
#define strchr index
#endif

#if !HAVE_STRRCHR && HAVE_RINDEX
#define strrchr rindex
#endif

/* Supported defines:
 * ULTRAGETOPT_LIKE_BSD		Behave like BSD getopt()
 * ULTRAGETOPT_LIKE_DARWIN	Behave like Darwin (Mac OS) getopt()
 * ULTRAGETOPT_LIKE_GNU		Behave like GNU getopt()
 * ULTRAGETOPT_LIKE_POSIX   	Behave like POSIX definition of getopt()
 *
 * ULTRAGETOPT_ASSIGNSPACE	Parse "-o value" as "value" rather than " value"
 *				Note: Only applicable when argv[x] == "-o value"
 *				      Not for argv[x] == "-o" [x+1] == "value"
 * ULTRAGETOPT_DEFAULTOPTOPT	Set optopt to this value by default on each
 *				call to getopt()
 * ULTRAGETOPT_HYPHENARG	Accept -option -arg as -option with argument
 *				"-arg" rather than -option missing argument
 * ULTRAGETOPT_LONGOPTADJACENT	Accept adjacent arguments to long options
 *				(e.g. --optionarg) based on first longest-match
 * ULTRAGETOPT_OPTIONPERMUTE	Permute options, do not stop at first non-option
 *				Behaves like GNU getopt where leading '+' or
 *				$POSIXLY_CORRECT both stop this @ runtime
 * ULTRAGETOPT_SHORTOPTASSIGN	Support -o=file syntax for short options
 * ULTRAGETOPT_SEPARATEDOPTIONAL Accept separated optional arguments
 *				 Parse -o file as -o  with argument file
 * ULTRAGETOPT_DOS_DASH		Support - and -- options in ultragetopt*_dos()
 * ULTRAGETOPT_BSD_ERRORS	Print error messages matching BSD getopt
 * ULTRAGETOPT_DARWIN_ERRORS	Print error messages matching Darwin getopt
 * ULTRAGETOPT_GNU_ERRORS	Print error messages matching GNU getopt
 * ULTRAGETOPT_NO_EATDASHDASH	Do not increment optind when argv[optind] is --
 *				as required by SUS/POSIX
 * ULTRAGETOPT_NO_OPTIONALARG	Do not support GNU "::" optional argument
 *				Always supported in *_long*()
 * ULTRAGETOPT_NO_OPTIONASSIGN	Do not support --option=value syntax
 */

#ifdef ULTRAGETOPT_LIKE_POSIX
#define ULTRAGETOPT_NO_OPTIONALARG
#define ULTRAGETOPT_NO_OPTIONASSIGN
#define ULTRAGETOPT_NO_EATDASHDASH
#undef ULTRAGETOPT_ASSIGNSPACE
#undef ULTRAGETOPT_BSD_ERRORS
#undef ULTRAGETOPT_DARWIN_ERRORS
#undef ULTRAGETOPT_GNU_ERRORS
#undef ULTRAGETOPT_OPTIONPERMUTE
#undef ULTRAGETOPT_SHORTOPTASSIGN
#elif defined(ULTRAGETOPT_LIKE_GNU)
#define ULTRAGETOPT_GNU_ERRORS
#define ULTRAGETOPT_HYPHENARG
#define ULTRAGETOPT_OPTIONPERMUTE
#undef ULTRAGETOPT_ASSIGNSPACE
#undef ULTRAGETOPT_NO_OPTIONALARG
#undef ULTRAGETOPT_NO_OPTIONASSIGN
#undef ULTRAGETOPT_NO_EATDASHDASH
#undef ULTRAGETOPT_SHORTOPTASSIGN
#undef ULTRAGETOPT_SEPARATEOPTIONAL
#undef ULTRAGETOPT_LONGOPTADJACENT
#elif defined(ULTRAGETOPT_LIKE_BSD)
#define ULTRAGETOPT_BSD_ERRORS
#define ULTRAGETOPT_OPTIONPERMUTE
#define ULTRAGETOPT_DEFAULTOPTOPT '?'
#undef ULTRAGETOPT_ASSIGNSPACE
#undef ULTRAGETOPT_NO_OPTIONALARG
#undef ULTRAGETOPT_NO_OPTIONASSIGN
#undef ULTRAGETOPT_NO_EATDASHDASH
#undef ULTRAGETOPT_SHORTOPTASSIGN
#undef ULTRAGETOPT_SEPARATEOPTIONAL
#undef ULTRAGETOPT_LONGOPTADJACENT
#elif defined(ULTRAGETOPT_LIKE_DARWIN)
#define ULTRAGETOPT_DARWIN_ERRORS
#define ULTRAGETOPT_OPTIONPERMUTE
#undef ULTRAGETOPT_ASSIGNSPACE
#undef ULTRAGETOPT_NO_OPTIONALARG
#undef ULTRAGETOPT_NO_OPTIONASSIGN
#undef ULTRAGETOPT_SHORTOPTASSIGN
#undef ULTRAGETOPT_NO_EATDASHDASH
#undef ULTRAGETOPT_SEPARATEOPTIONAL
#undef ULTRAGETOPT_LONGOPTADJACENT
#endif

#ifdef ULTRAGETOPT_NO_OPTIONASSIGN
static const char *const unixassigners = "";
// static const char *const dosassigners = ":";
#elif defined(ULTRAGETOPT_OPTIONSPACE)
static const char *const unixassigners = "= ";
// static const char *const dosassigners = ":= ";
#else
static const char *const unixassigners = "=";
// static const char *const dosassigners = ":=";
#endif

#ifdef ULTRAGETOPT_DOS_DASH
static const char *const unixleaders = "-";
// static const char *const dosleaders = "/-";
#else
static const char *const unixleaders = "-";
// static const char *const dosleaders = "/";
#endif

/* Flags for all variants of ultragetopt*() */
static const int getoptflags = 0
#ifdef ULTRAGETOPT_SEPARATEDOPTIONAL
                               | UGO_SEPARATEDOPTIONAL
#endif
#ifdef ULTRAGETOPT_SHORTOPTASSIGN
                               | UGO_SHORTOPTASSIGN
#endif
#ifdef ULTRAGETOPT_NO_EATDASHDASH
                               | UGO_NOEATDASHDASH
#endif
#ifdef ULTRAGETOPT_HYPHENARG
                               | UGO_HYPHENARG
#endif
    ;

#ifdef ULTRAGETOPT_GNU_ERRORS
static const char *const errorarg =
    "%s: option `%.*s' doesn't allow an argument\n";
static const char *const errornoarg =
    "%s: option requires an argument -- `%.*s'\n";
static const char *const erroropt = "%s: unrecognized option `%.*s'\n";
static const char *const errorargc =
    "%s: option `-%c' does not take an argument\n";
static const char *const errornoargc =
    "%s: option requires an argument -- `-%c'\n";
static const char *const erroroptc = "%s: invalid option -- %c\n";
#elif defined(ULTRAGETOPT_BSD_ERRORS)
static const char *const errorarg =
    "%s: option doesn't take an argument -- %.*s\n";
static const char *const errornoarg =
    "%s: option requires an argument -- %.*s\n";
static const char *const erroropt = "%s: unknown option -- %.*s\n";
static const char *const errorargc =
    "%s: option doesn't take an argument -- %c\n";
static const char *const errornoargc =
    "%s: option requires an argument -- %c\n";
static const char *const erroroptc = "%s: unknown option -- %c\n";
#elif defined(ULTRAGETOPT_DARWIN_ERRORS)
static const char *const errorarg =
    "%s: option `%.*s' doesn't allow an argument\n"; /* with -- */
static const char *const errornoarg =
    "%s: option `%.*s' requires an argument\n";
static const char *const erroropt =
    "%s: unrecognized option `%.*s'\n"; /* with -- */
static const char *const errorargc =
    "%s: option doesn't take an argument -- %c\n";
static const char *const errornoargc =
    "%s: option requires an argument -- %c\n";
static const char *const erroroptc = "%s: invalid option -- %c\n";
#else /* POSIX-like */
static const char *const errorarg =
    "%s: option does not take an argument -- %.*s\n";
static const char *const errornoarg =
    "%s: option requires an argument -- %.*s\n";
static const char *const erroropt = "%s: illegal option -- %.*s\n";
static const char *const errorargc =
    "%s: option does not take an argument -- %c\n";
static const char *const errornoargc =
    "%s: option requires an argument -- %c\n";
static const char *const erroroptc = "%s: illegal option -- %c\n";
#endif

/* to match optarg, optind, opterr, optopt, optreset */
GetOpt::GetOpt()
    : ultraoptarg(0), ultraoptind(1), ultraopterr(1), ultraoptreset(0),
#ifdef ULTRAGETOPT_DEFAULTOPTOPT
      ultraoptopt(ULTRAGETOPT_DEFAULTOPTOPT - 0)
#else
      ultraoptopt(0)
#endif
{
}

static int ultraoptnum = 0; /* How many options of the current multi-option
                               argument have been processed?  (e.g. -vvv) */

/* Print errors only if not suppressed */
void GetOpt::print_error(int flags, const char *templat, ...)
{
  va_list ap;

  va_start(ap, templat);
  if (ultraopterr != 0 && !(flags & UGO_NOPRINTERR))
    vfprintf(stderr, templat, ap);
  va_end(ap);
}

/* If argv[curopt] matches a long option, return the index of that option
 * Otherwise, return -1
 * If it has an adjacent argument, return pointer to it in longarg, else 0
 */
static int match_longopt(int curopt, char *const argv[],
                         const struct option *longopts, const char *assigners,
                         const char *optleaders, int flags, char **longarg)
{
  size_t alen, optnamelen = 0;
  char *optname;
  char *temp;
  int i;
  int (*optncmp)(const char *s1, const char *s2, size_t n);

  if (longarg == 0)
    longarg = &temp;
  *longarg = 0;

  if (flags & UGO_CASEINSENSITIVE)
    optncmp = strncasecmp;
  else
    optncmp = strncmp;

  if (longopts == 0)
    return -1;

  /* Not an option */
  if (argv[curopt][0] == '\0' || argv[curopt][1] == '\0' ||
      !strchr(optleaders, argv[curopt][0]))
    return -1;

  if (flags & UGO_SINGLELEADERONLY) {
    optname = argv[curopt] + 1;
  }
  else if (!strchr(optleaders, argv[curopt][1])) {
    /* Possible short option */
    if (flags & UGO_SINGLELEADERLONG)
      optname = argv[curopt] + 1;
    else
      return -1;
  }
  else {
    optname = argv[curopt] + 2;
  }

  /* Do first longest-match search if requested */
  if (flags & UGO_LONGOPTADJACENT) {
    size_t matchlen = 0;
    int matchind = -1;
    for (i = 0; longopts[i].name != 0; i++) {
      size_t longnamelen = strlen(longopts[i].name);
      if (longnamelen > matchlen &&
          optncmp(optname, longopts[i].name, longnamelen) == 0) {
        matchind = i;
        matchlen = longnamelen;
      }
    }

    if (matchlen > 0) {
      /* See if our match has an adjacent argument */
      if (optname[matchlen] != '\0') {
        /* Strip assigner character if present */
        if (strchr(assigners, optname[matchlen]))
          *longarg = optname + matchlen + 1;
        else
          *longarg = optname + matchlen;
      }

      return matchind;
    }

    return -1;
  }

  /* Check for assigner in the option */
  alen = strlen(assigners);
  for (i = 0; (unsigned)i < alen; i++) {
    char *asn = strchr(optname, assigners[i]);
    if (asn != 0) {
      optnamelen = asn - optname;
      *longarg = asn + 1;
      break;
    }
  }

  if (optnamelen == 0)
    optnamelen = strlen(optname);

  for (i = 0; longopts[i].name != 0; i++)
    if (optncmp(optname, longopts[i].name, optnamelen) == 0 &&
        strlen(longopts[i].name) == optnamelen)
      return i;

  return -1;
}

/* Check if an option has a separate argument (in the following argv[] index) */
int GetOpt::has_separate_argument(int curopt, int argc, char *const argv[],
                                  const char *shortopts,
                                  const struct option *longopts,
                                  const char *assigners, const char *optleaders,
                                  int flags)
{
  int longind;
  char *longarg;

  assert(curopt < argc && argv[curopt][0] != '\0' &&
         strchr(optleaders, argv[curopt][0]));

  /* Check if we have a long option */
  longind = match_longopt(ultraoptind, argv, longopts, assigners, optleaders,
                          flags, &longarg);
  if (longind >= 0) {
    if (longopts[longind].has_arg == no_argument || longarg != 0 ||
        (longopts[longind].has_arg == optional_argument &&
         !(flags & UGO_SEPARATEDOPTIONAL)))
      return 0;

    return argv[curopt + 1] != 0           /* Argument exists */
           && (argv[curopt + 1][0] == '\0' /* and is an argument */
               || ((flags & UGO_HYPHENARG) /* takes hyphenated */
                   && longopts[longind].has_arg == required_argument) ||
               !strchr(optleaders, argv[curopt + 1][0])); /* not -arg */
  }
  else if (!strchr(optleaders, argv[curopt][1])) {
    /* Short option */
    const char *optpos;

    optpos = strchr(shortopts, argv[curopt][1]);
    if ((flags & UGO_CASEINSENSITIVE) && optpos == 0) {
      if (islower(argv[curopt][1]))
        optpos = strchr(shortopts, toupper(argv[curopt][1]));
      else
        optpos = strchr(shortopts, tolower(argv[curopt][1]));
    }

    return optpos != 0         /* Option found */
           && optpos[1] == ':' /* Option takes argument */
           && (optpos[2] != ':' || (flags & UGO_SEPARATEDOPTIONAL)) &&
           argv[curopt][2] == '\0'         /* Argument is not adjacent */
           && argv[curopt + 1] != 0        /* Argument exists */
           && (argv[curopt + 1][0] == '\0' /* Is an argument */
               || ((flags & UGO_HYPHENARG) && optpos[2] != ':') ||
               !strchr(optleaders, argv[curopt + 1][0]));
  }

  /* No match */
  return 0;
}

/* Bring the next option up to ultraoptind if there is one
 * Returns number of words shifted forward
 */
int GetOpt::permute_options(int argc, char *argv[], const char *shortopts,
                            const struct option *longopts,
                            const char *assigners, const char *optleaders,
                            int flags)
{
  int curopt = ultraoptind;

  /* If we already have an option or no more possible, give up */
  if (curopt >= argc || (argv[curopt][0] != '\0' && argv[curopt][1] != '\0' &&
                         strchr(optleaders, argv[curopt][0])))
    return 0;

  for (; curopt < argc && argv[curopt]; curopt++) {
    int shiftarg = 0;
    int i;

    /* Skip non-options */
    if (argv[curopt][0] == '\0' || argv[curopt][1] == '\0' ||
        !strchr(optleaders, argv[curopt][0]))
      continue;

    /* Check if we need to shift argument too */
    shiftarg = has_separate_argument(curopt, argc, argv, shortopts, longopts,
                                     assigners, optleaders, flags);

    /* Shift option */
    for (i = curopt; i > ultraoptind; i--) {
      char *temp = argv[i];
      argv[i] = argv[i - 1];
      argv[i - 1] = temp;

      if (shiftarg) {
        temp = argv[i + 1];
        argv[i + 1] = argv[i];
        argv[i] = temp;
      }
    }

    /* All done */
    if (shiftarg)
      return 2;
    else
      return 1;
  }

  /* Couldn't find an option, bummer */
  return 0;
}

/* Handle a longopts[longind] matches argv[ultraoptind] actions */
int GetOpt::handle_longopt(int longind, char *longarg, int noseparg,
                           char *const argv[], const struct option *longopts,
                           int *indexptr, const char *optleaders, int flags)
{
  /* Handle assignment arguments */
  if (longarg && longopts[longind].has_arg == no_argument) {
    print_error(flags, errorarg, argv[0],
                (int)(longarg - argv[ultraoptind] - 1), argv[ultraoptind]);
    /* TODO:  What is a good value to put in ultraoptopt? */
    /* Looks like GNU getopt() uses val */
    ultraoptopt = longopts[longind].val;
    ultraoptind++;
    return '?';
  }
  else if (longarg) {
    ultraoptind++;
    ultraoptarg = longarg;

    if (indexptr)
      *indexptr = longind;

    if (longopts[longind].flag) {
      *(longopts[longind].flag) = longopts[longind].val;
      return 0;
    }
    else
      return longopts[longind].val;
  }

  /* Handle missing required argument */
  if (longopts[longind].has_arg == required_argument &&
      (noseparg || argv[ultraoptind + 1] == 0 ||
       (!(flags & UGO_HYPHENARG) &&
        strchr(optleaders, argv[ultraoptind + 1][0])))) {
    print_error(flags, errornoarg, argv[0], (int)strlen(argv[ultraoptind]),
                argv[ultraoptind]);
    ultraoptind++;
    if (flags & UGO_MISSINGCOLON)
      return ':';
    else
      return '?';
  }

  /* Handle available argument */
  if ((longopts[longind].has_arg == required_argument ||
       (longopts[longind].has_arg == optional_argument &&
        (flags & UGO_SEPARATEDOPTIONAL))) &&
      !noseparg && argv[ultraoptind + 1] != 0 &&
      (argv[ultraoptind + 1][0] == '\0' ||
       ((flags & UGO_HYPHENARG) &&
        longopts[longind].has_arg == required_argument) ||
       !strchr(optleaders, argv[ultraoptind + 1][0]))) {
    ultraoptarg = argv[ultraoptind + 1];
    ultraoptind += 2;
  }
  else
    ultraoptind++;

  if (indexptr)
    *indexptr = longind;

  if (longopts[longind].flag) {
    *(longopts[longind].flag) = longopts[longind].val;
    return 0;
  }
  else
    return longopts[longind].val;
}

int GetOpt::ultragetopt_tunable(int argc, char *const argv[],
                                const char *shortopts,
                                const struct option *longopts, int *indexptr,
                                const char *assigners, const char *optleaders,
                                int flags)
{
  char *opt;          /* Option we are processing */
  const char *optpos; /* Pointer to opt in shortopts */
  int noseparg = 0;   /* Force option not to have a separate argument */

  if (!shortopts)
    shortopts = ""; // AR: pointer mustn't be 0

  if (ultraoptreset) {
    ultraoptind = 1;
    ultraopterr = 1;
    ultraoptnum = 0;
    ultraoptreset = 0;
  }

  ultraoptarg = 0;
#ifdef ULTRAGETOPT_DEFAULTOPTOPT
  ultraoptopt = ULTRAGETOPT_DEFAULTOPTOPT - 0;
#endif

  /* Sanity check (These are specified verbatim in SUS) */
  /* AR: changed > to >= to allow for argv with trailing valid options */
  if (ultraoptind >= argc || argv[ultraoptind] == 0)
    return -1;

  /* No permuting when $POSIXLY_CORRECT is set (to match GNU getopt) */
  if (getenv("POSIXLY_CORRECT"))
    flags &= ~UGO_OPTIONPERMUTE;

  /* Get flags from shortopts */
  for (; shortopts && *shortopts; shortopts++) {
    if (*shortopts == '+')
      flags &= ~UGO_OPTIONPERMUTE;
    else if (*shortopts == '-')
      flags |= UGO_NONOPTARG;
    else if (*shortopts == ':') {
      flags |= UGO_NOPRINTERR;
      flags |= UGO_MISSINGCOLON;
    }
    else
      break;
  }

  /* Found non-option */
  if (argv[ultraoptind][0] == '\0' || argv[ultraoptind][1] == '\0' ||
      !strchr(optleaders, argv[ultraoptind][0])) {
    int shifted;

    if (flags & UGO_NONOPTARG) {
      ultraoptarg = argv[ultraoptind];
      ultraoptind++;
      return 1;
    }

    if (!(flags & UGO_OPTIONPERMUTE))
      return -1;

    shifted = permute_options(argc, (char **)argv, shortopts, longopts,
                              assigners, optleaders, flags);
    if (shifted == 0)
      return -1;
    else if (shifted == 1)
      noseparg = 1;
  }

  /* At this point we must have an option of some sort */
  assert(strchr(optleaders, argv[ultraoptind][0]));

  /* Handle -- */
  if (argv[ultraoptind][0] == argv[ultraoptind][1]) {
    int longind;
    char *longarg;

    /* End of options signaled by string of 2 leaders alone ("--") */
    if (argv[ultraoptind][2] == '\0') {
      if (!(flags & UGO_NOEATDASHDASH))
        ultraoptind++;

      return -1;
    }

    /* Handle long option */
    longind = match_longopt(ultraoptind, argv, longopts, assigners, optleaders,
                            flags, &longarg);
    if (longind < 0) {
      if (longarg == 0)
        print_error(flags, erroropt, argv[0], (int)strlen(argv[ultraoptind]),
                    argv[ultraoptind]);
      else
        print_error(flags, erroropt, argv[0],
                    (int)(longarg - argv[ultraoptind] - 1), argv[ultraoptind]);

      /* TODO:  What is a good value for optopt in this case? */
      /*	      Looks like BSD uses 0 */
      ultraoptopt = 0;
      ultraoptind++;
      return '?';
    }

    return handle_longopt(longind, longarg, noseparg, argv, longopts, indexptr,
                          optleaders, flags);
  }

  /* See if it matches a long-only option */
  if (longopts != 0 && ultraoptnum == 0 &&
      ((flags & UGO_SINGLELEADERLONG) || (flags & UGO_SINGLELEADERONLY))) {
    int longind;
    char *longarg;

    longind = match_longopt(ultraoptind, argv, longopts, assigners, optleaders,
                            flags, &longarg);

    if (longind >= 0)
      return handle_longopt(longind, longarg, noseparg, argv, longopts,
                            indexptr, optleaders, flags);
  }

  /* No long matches, process short option */
  opt = argv[ultraoptind] + ultraoptnum + 1;
  optpos = strchr(shortopts, opt[0]);
  if (optpos == 0 && (flags & UGO_CASEINSENSITIVE)) {
    if (islower(opt[0]))
      optpos = strchr(shortopts, toupper(opt[0]));
    else
      optpos = strchr(shortopts, tolower(opt[0]));
  }

  /* This could indicate ultraoptnum not being reset properly */
  assert(opt[0] != '\0');

  /* Check for invalid or unrecognized option */
  if (optpos == 0 || opt[0] == ':') {
    print_error(flags, erroroptc, argv[0], opt[0]);

    ultraoptopt = opt[0];
    if (opt[1] != '\0')
      ultraoptnum++;
    else {
      ultraoptnum = 0;
      ultraoptind++;
    }

    return '?';
  }

  /* Handle arguments */
  if (optpos[1] == ':') {
    ultraoptnum = 0;

    /* Handle adjacent arguments -ofile.txt */
    if (opt[1] != '\0') {
      /* Skip over assignment character */
      if ((flags & UGO_SHORTOPTASSIGN) && strchr(assigners, opt[1]))
        ultraoptarg = opt + 2;
      else
        ultraoptarg = opt + 1;

      ultraoptind++;
      return optpos[0];
    }

    /* Handle optional argument not present */
    if ((flags & UGO_OPTIONALARG)               /* accept optionals */
        && optpos[2] == ':'                     /* opt takes optional */
        && (argv[ultraoptind + 1] == 0          /* optional doesn't exist */
            || !(flags & UGO_SEPARATEDOPTIONAL) /* separated accepted */
            || strchr(optleaders, argv[ultraoptind + 1][0]))) {
      ultraoptind++;
      return optpos[0];
    }

    /* Handle separated argument missing */
    if (ultraoptind + 2 > argc || noseparg || argv[ultraoptind + 1] == 0 ||
        (!(flags & UGO_HYPHENARG) &&
         strchr(optleaders, argv[ultraoptind + 1][0]))) {
      ultraoptind++;
      print_error(flags, errornoargc, argv[0], opt[0]);

      ultraoptopt = opt[0];
      if (flags & UGO_MISSINGCOLON)
        return ':';
      else
        return '?';
    }

    ultraoptind += 2;
    ultraoptarg = argv[ultraoptind - 1];
    return optpos[0];
  }

  /* Handle argumentless option with assigned option */
  if ((flags & UGO_SHORTOPTASSIGN) && opt[1] != '\0' &&
      strchr(assigners, opt[1])) {
    print_error(flags, errorargc, argv[0], opt[0]);
    ultraoptnum = 0;
    ultraoptopt = opt[0];
    ultraoptind++;
    return '?';
  }

  if (opt[1] != '\0') {
    ultraoptnum++;
  }
  else {
    ultraoptnum = 0;
    ultraoptind++;
  }

  return optpos[0];
}

/* POSIX-compliant getopt
 *
 * Handles optional argument '::' specifier as an extension for compatibility
 * with glibc
 */
int GetOpt::ultragetopt(int argc, char *const argv[], const char *optstring)
{
  int flags = getoptflags;

#ifdef ULTRAGETOPT_OPTIONPERMUTE
  flags |= UGO_OPTIONPERMUTE;
#endif

#ifndef ULTRAGETOPT_NO_OPTIONALARG
  flags |= UGO_OPTIONALARG;
#endif

  return ultragetopt_tunable(argc, argv, optstring, 0, 0, unixassigners,
                             unixleaders, flags);
}

/* GNU getopt_long workalike
 *
 * Argument reordering not yet implemented
 * Leading + and - under consideration (behavior violates POSIX...)
 */
int GetOpt::ultragetopt_long(int argc, char *const argv[],
                             const char *shortopts,
                             const struct option *longopts, int *indexptr)
{
  return ultragetopt_tunable(argc, argv, shortopts, longopts, indexptr,
                             unixassigners, unixleaders,
                             getoptflags | UGO_OPTIONPERMUTE | UGO_OPTIONALARG);
}

/* GNU getopt_long_only workalike */
int GetOpt::ultragetopt_long_only(int argc, char *const argv[],
                                  const char *shortopts,
                                  const struct option *longopts, int *indexptr)
{
  return ultragetopt_tunable(
      argc, argv, shortopts, longopts, indexptr, unixassigners, unixleaders,
      getoptflags | UGO_SINGLELEADERLONG | UGO_OPTIONPERMUTE | UGO_OPTIONALARG);
}
/*
int GetOpt::ultragetopt_dos(int argc, char * const argv[], const char
*optstring)
{
   return ultragetopt_tunable(argc, argv, optstring, 0, 0,
         dosassigners, dosleaders,
         getoptflags | UGO_CASEINSENSITIVE);
}

int GetOpt::ultragetopt_long_dos(int argc, char *const argv[], const char
*shortopts,
      const struct option *longopts, int *indexptr)
{
   return ultragetopt_tunable(argc, argv, shortopts, longopts, indexptr,
         dosassigners, dosleaders,
         getoptflags | UGO_CASEINSENSITIVE
         | UGO_SINGLELEADERLONG | UGO_SINGLELEADERONLY
         | UGO_OPTIONPERMUTE | UGO_OPTIONALARG);
}
*/
/* vim:set sts=4 sw=4: */


AC_PREREQ([2.69])
AC_INIT([mpd_oled], [0.2], [adrian@antiprism.com])
AM_INIT_AUTOMAKE([1.9 foreign])
LT_INIT

AC_CONFIG_SRCDIR([src/glcdfont.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Set compiler
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX([11], [noext])

# Add -fno-strict-aliasing to CFLAGS if accepted
if $CXX -fno-strict-aliasing -S -o /dev/null -xc /dev/null >/dev/null 2>&1;
then
   CPPFLAGS="-fno-strict-aliasing $CPPFLAGS"
fi

# For libmpdclient
AC_DEFINE([DEFAULT_PORT], [6600], [Default port for libmpdclient])
AC_DEFINE([DEFAULT_HOST], ["localhost"], [Default host for libmpdclient])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h locale.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([gettimeofday memset mkfifo munmap select setlocale socket strerror strspn strstr strtol strndup])

inst_msg="Install a development version of this library and rerun ./configure"
AC_LANG_PUSH([C++])

AC_ARG_VAR(LIBMPDCLIENT_DIR, ["libmpdclient source directory, built into subdirectory called output"])
AC_MSG_CHECKING([LIBMPDCLIENT_DIR])
tmp_dir=tmp_libmpdclient
rm -rf ${tmp_dir}
if test ! x"$LIBMPDCLIENT_DIR" = x"" ; then
  AC_MSG_RESULT([$LIBMPDCLIENT_DIR (using locally built libmpdclient)])
  mkdir ${tmp_dir}
  cp -a ${LIBMPDCLIENT_DIR}/include $tmp_dir
  cp -a ${LIBMPDCLIENT_DIR}/output/version.h ${tmp_dir}/include/mpd
  cp -a ${LIBMPDCLIENT_DIR}/output/libmpdclient.a ${tmp_dir}
  AC_SUBST(LIBMPDCLIENT_TMP_DIR, [$tmp_dir])
else
  AC_MSG_RESULT([no (using installed libmpdclient)])
  AC_CHECK_HEADER([mpd/client.h], [], [AC_MSG_ERROR([$inst_msg])])
  AC_CHECK_LIB([mpdclient], [mpd_connection_new], [], [AC_MSG_ERROR([$inst_msg])])
fi
AM_CONDITIONAL([LIBMPDCLIENT_LOCAL], [test ! x"$LIBMPDCLIENT_DIR" = x""])

#AC_ARG_VAR(LIBUEV_DIR, ["libuev source directory"])
#AC_MSG_CHECKING([LIBUEV_DIR])
#tmp_dir=tmp_libuev
#rm -rf ${tmp_dir}
#if test ! x"$LIBUEV_DIR" = x"" ; then
#  AC_MSG_RESULT([$LIBUEV_DIR (using locally built libmpdclient)])
#  mkdir ${tmp_dir} ${tmp_dir}/uev
#  cp -a ${LIBUEV_DIR}/src/uev.h ${LIBUEV_DIR}/src/private.h $tmp_dir/uev
#  cp -a ${LIBUEV_DIR}/src/.libs/libuev.a ${tmp_dir}
### AC_SUBST(LIBUEV_TMP_DIR, [$tmp_dir])
#else
#  AC_MSG_RESULT([no (using installed libuev)])
#  AC_CHECK_HEADER([uev/uev.h], [], [AC_MSG_ERROR([$inst_msg])])
#  AC_CHECK_LIB([uev], [uev_init], [], [AC_MSG_ERROR([$inst_msg])])
#fi
#AM_CONDITIONAL([LIBUEV_LOCAL], [test ! x"$LIBUEV_DIR" = x""])


# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_ERROR([$inst_msg])])
AC_CHECK_HEADER([pthread.h], [], [AC_MSG_ERROR([$inst_msg])])
#AC_CHECK_LIB([i2c], [i2c_smbus_write_byte_data], [], [])

AC_LANG_POP([C++])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/hjson_cpp/Makefile
                 src/http_tiny/Makefile
                 scripts/mpd_oled_service_install
                 ])

AC_OUTPUT

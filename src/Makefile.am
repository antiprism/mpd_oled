SUBDIRS = hjson_cpp http_tiny

bin_PROGRAMS = mpd_oled

mpd_oled_SOURCES = \
	controller.cpp display.cpp main.cpp player.cpp programopts.cpp \
	status.cpp status_msg.cpp timer.cpp ultragetopt.cpp utils.cpp \
	u8x8_d_sdl.c \
	\
        controller.h display.h display_info.h \
	iconv_wrap.h player.h programopts.h status.h \
	status_msg.h timer.h ultragetopt.h utils.h \
	u8x8_d_sdl.h

mpd_oled_LDADD = \
	hjson_cpp/libhjsoncpp.la \
	http_tiny/libhttptiny.la

mpd_oled_LDFLAGS = -static-libstdc++

AM_CPPFLAGS =
if LIBMPDCLIENT_LOCAL
   mpd_oled_LDADD += $(top_srcdir)/$(LIBMPDCLIENT_TMP_DIR)/libmpdclient.a
   AM_CPPFLAGS += -I$(top_srcdir)/$(LIBMPDCLIENT_TMP_DIR)/include
endif

if LIBU8G2_LOCAL
   mpd_oled_LDADD += $(top_srcdir)/$(LIBU8G2_TMP_DIR)/libu8g2arm.a
   AM_CPPFLAGS += -I$(top_srcdir)/$(LIBU8G2_TMP_DIR)/include
endif


#if LIBUEV_LOCAL
#   mpd_oled_LDADD += $(top_srcdir)/$(LIBUEV_TMP_DIR)/libuev.a
#   AM_CPPFLAGS += -I$(top_srcdir)/$(LIBUEV_TMP_DIR)



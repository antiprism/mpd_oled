SUBDIRS = hjson_cpp http_tiny

bin_PROGRAMS = mpd_oled

mpd_oled_SOURCES = \
	bcm2835.c bcm2835_i2c.c glcdfont.c \
	\
	Adafruit_GFX.cpp ArduiPi_OLED.cpp display.cpp \
	main.cpp player.cpp programopts.cpp status.cpp \
	status_msg.cpp timer.cpp ultragetopt.cpp utils.cpp \
	\
	Adafruit_GFX.h ArduiPi_OLED.h ArduiPi_OLED_lib.h \
	bcm2835.h bcm2835_i2c.h display.h display_info.h \
	gfxfont.h iconv_wrap.h player.h programopts.h status.h \
	status_msg.h timer.h ultragetopt.h utils.h

mpd_oled_LDADD = \
	hjson_cpp/libhjsoncpp.la \
	http_tiny/libhttptiny.la

mpd_oled_LDFLAGS = -static-libstdc++

AM_CPPFLAGS =
if LIBMPDCLIENT_LOCAL
   mpd_oled_LDADD += $(top_srcdir)/$(LIBMPDCLIENT_TMP_DIR)/libmpdclient.a
   AM_CPPFLAGS += -I$(top_srcdir)/$(LIBMPDCLIENT_TMP_DIR)/include
endif

#if LIBUEV_LOCAL
#   mpd_oled_LDADD += $(top_srcdir)/$(LIBUEV_TMP_DIR)/libuev.a
#   AM_CPPFLAGS += -I$(top_srcdir)/$(LIBUEV_TMP_DIR)



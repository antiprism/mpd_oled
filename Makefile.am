ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

EXTRA_DIST = README.md NEWS AUTHORS COPYING mpd_oled.jpg \
	bootstrap commands.txt \
	\
	doc scripts

docdir = @docdir@
doc_DATA = README.md NEWS AUTHORS COPYING mpd_oled.jpg

docdocdir = ${docdir}/doc
docdoc_DATA = \
	doc/FAQ.md \
	doc/install_moode7_deb.md \
	doc/install_moode7_source.md \
	doc/install_moode6_deb.md \
	doc/install_moode6_source.md \
	doc/install_volumio2_deb.md \
	doc/install_volumio2_source.md \
	doc/install_raudio1_zst.md \
	doc/install_raudio1_source.md \
	doc/wiring_i2c.png \
	doc/wiring_spi.png


filesdir = ${datarootdir}/${PACKAGE}
files_DATA = scripts/mpd_oled_fifo.conf

moodedir = ${filesdir}/moode
moode_DATA = \
	scripts/moode6_mpd_fifo.patch \
	scripts/moode7_mpd_fifo.patch \
	scripts/moode_mpd_fifo_old.patch

bin_SCRIPTS = \
	scripts/mpd_oled_service_install \
	scripts/mpd_oled_service_edit \
	scripts/mpd_oled_service_uninstall \
	scripts/mpd_oled_volumio_mpd_conf_install \
	scripts/mpd_oled_volumio_mpd_conf_uninstall \
	scripts/mpd_oled_raudio_mpd_conf_install \
	scripts/mpd_oled_raudio_mpd_conf_uninstall \
	scripts/mpd_oled_usrlocal_check \
	scripts/mpd_oled_usrlocal_uninstall

format_all:
	for f in src/*.cpp src/*.h ; do \
	clang-format -style=file -i $$f; \
	done


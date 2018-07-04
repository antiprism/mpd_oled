ifeq ($(PLAYER),)
	PLAYER=MPD
	# PLAYER=MPD # MPD VOLUMIO RUNEAUDIO
endif

ifeq ($(PLAYER),VOLUMIO)
	PLAYERLIBS=-lcurl -ljsoncpp
else ifeq ($(PLAYER),RUNEAUDIO)
	PLAYERLIBS=-li2c
endif

CPPFLAGS=-W -Wall -Wno-unused-variable -Wno-unused-parameter \
	 -Wno-strict-aliasing -Ofast -D$(PLAYER)

PROG_NAME=mpd_oled
includes = $(wildcard *.h)

# make all
all: $(PROG_NAME)

# Make the library
OBJECTS=main.o timer.o status.o status_msg.o utils.o display.o\
	programopts.o ultragetopt.o \
	ArduiPi_OLED.o Adafruit_GFX.o bcm2835.o
LDLIBS=-lmpdclient -lpthread $(PLAYERLIBS)
$(OBJECTS): $(includes)
$(PROG_NAME): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)


PROG_NAME2=audio_fifos
OBJECTS2=audio_multi_fifo.o status_msg.o utils.o programopts.o \
         ultragetopt.o
$(OBJECTS2): $(includes)
$(PROG_NAME2): $(OBJECTS2)
	$(CXX) $(LDFLAGS) -o $@ $^

# clear build files
clean:
	rm -rf *.o $(PROG_NAME) $(PROG_NAME2)

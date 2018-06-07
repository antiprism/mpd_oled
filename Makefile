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
OBJECTS=main.o timer.o status.o ArduiPi_OLED.o Adafruit_GFX.o \
	bcm2835.o display.o
LDLIBS=-lmpdclient -lpthread $(PLAYERLIBS)
$(OBJECTS): $(includes)
$(PROG_NAME): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# clear build files
clean:
	rm -rf *.o $(PROG_NAME)

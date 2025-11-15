# Makefile for building embedded application.
# by Brian Fraser 
# Updated by Matthew Stewart

# Edit this file to compile extra C files into their own programs.
TARGET= audioTest
SOURCES= audioMixer.c audioMixerTest.c

PUBDIR = $(HOME)/ensc351/public/myApps
OUTDIR = $(PUBDIR)
CROSS_TOOL = aarch64-linux-gnu-
CC_CPP = $(CROSS_TOOL)g++
CC_C = $(CROSS_TOOL)gcc

CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L 
# -Werror removed so that the unused variable don't block the compile.  You should put this back in.
# you will want to use the unused variables.

# -pg for supporting gprof profiling.
#CFLAGS += -pg


all: audioTest wav
	@echo
	@echo '******************************************'
	@echo '***             DONE                   ***'
	@echo '******************************************'
	@echo

install-asound:
	# Do this on the host
	sudo apt-get update
	sudo apt-get install libasound2-dev

	# Add cross-compile support for libasound
	sudo dpkg --add-architecture arm64
	sudo apt-get update
	sudo apt-get install libasound2-dev:arm64


audioTest:
	@echo
	@echo '******************************************'
	@echo '*** If missing alsa/asoundlib.h, run   ***'
	@echo '***       make install-asound          ***'
	@echo '******************************************'
	@echo
	$(CC_C) $(CFLAGS) $(SOURCES) -o $(OUTDIR)/$(TARGET)  $(LFLAGS) -lpthread -lasound

wav:
	# Copy wave files to the shared folder
	mkdir -p $(PUBDIR)/wave-files/
	cp wave-files/* $(PUBDIR)/wave-files/ 

clean:
	rm -f $(OUTDIR)/$(TARGET)

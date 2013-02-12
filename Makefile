#Makefile for utilities interacting with temperme.com hardware
#Copyright (C) 2011  Ihsan Kehribar <ihsan@kehribar.me>
#Copyright (C) 2012  Marcel Hecko <maco@blava.net> 
#Copyright (C) 2012  Michal Belica 
#
#This program is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public License
#as published by the Free Software Foundation; either version 2
#of the License, or (at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

CC=gcc

# FIXME: Need to add OSX stuff
ifeq ($(shell uname), Linux)
	USBFLAGS = `libusb-config --cflags`
	USBLIBS = `libusb-config --libs`
	EXE_SUFFIX = 
	OSFLAG = -D LINUX
    BIN_DIR = bin
    SBIN_DIR = bin
else
	USBFLAGS = -I C:\MinGW\include
	USBLIBS = -L C:\MinGW\lib -lusb
	EXE_SUFFIX = .exe
	OSFLAG = -D WIN
endif

LIBS    = $(USBLIBS)
INCLUDE = library
CFLAGS  = $(USBFLAGS) $(LIBS) -I$(INCLUDE) -O -g $(OSFLAG)

LWLIBS = opendevice littleWire littleWire_util 
SERVERCODE = usbenum
EXAMPLES = tempermed temperme_client
LDLIBS = -L/usr/lib/x86_64-linux-gnu -lcurl

.PHONY:	clean library

all: library tempermed temperme_client

library: $(LWLIBS)

$(LWLIBS):
	@echo Building library: $@...
	$(CC) $(CFLAGS) $(LDLIBS) -c library/$@.c 

$(addsuffix .o,$(SERVERCODE)): $(addprefix src/, $(addsuffix .c, $(SERVERCODE)))
	@echo Building server object files: $@...
	$(CC) $(CFLAGS) -c src/$(subst .o,.c,$@)

tempermed: $(addsuffix .o, $(LWLIBS)) $(addsuffix .o, $(SERVERCODE))
	@echo Building server: $@...
	$(CC) $(CFLAGS) -D_GNU_SOURCE=1 -pthread -Wno-unused-result -o $(SBIN_DIR)/$@$(EXE_SUFFIX) src/$@.c $^ $(LIBS) $(LDLIBS)

temperme_client:
	@echo Building client: $@...
	$(CC) $(CFLAGS) -D_GNU_SOURCE=1 -Wno-unused-result -o $(BIN_DIR)/$@$(EXE_SUFFIX) src/$@.c $^

clean:
	rm -f $(BIN_DIR)/* $(SBIN_DIR)/* *.o *.exe

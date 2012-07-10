# Makefile for Little Temp

CC=gcc

# FIXME: Need to add OSX stuff
ifeq ($(shell uname), Linux)
	USBFLAGS = `libusb-config --cflags`
	USBLIBS = `libusb-config --libs`
	EXE_SUFFIX =
	OSFLAG = -D LINUX
else
	USBFLAGS = -I C:\MinGW\include
	USBLIBS = -L C:\MinGW\lib -lusb
	EXE_SUFFIX = .exe
	OSFLAG = -D WIN
endif

LIBS    = $(USBLIBS)
INCLUDE = library
CFLAGS  = $(USBFLAGS) $(LIBS) -I$(INCLUDE) -O -g $(OSFLAG)

LWLIBS = opendevice littleWire littleWire_util littleWire_servo
SERVEROBJS = usbenum.o
EXAMPLES = temp_server temp_client
LDLIBS = -L/usr/lib/x86_64-linux-gnu -lcurl

.PHONY:	clean library

all: library temp_server temp_client

library: $(LWLIBS)

$(LWLIBS):
	@echo Building library: $@...
	$(CC) $(CFLAGS) $(LDLIBS) -c library/$@.c 

$(SERVEROBJS):
	@echo Building server object files: $@...
	$(CC) $(CFLAGS) -c src/$(subst .o,.c,$@)

temp_server: $(addsuffix .o, $(LWLIBS)) usbenum.o
	@echo Building server: $@...
	$(CC) $(CFLAGS) -D_GNU_SOURCE=1 -pthread -Wno-unused-result -o $@$(EXE_SUFFIX) src/$@.c $^ $(LIBS) $(LDLIBS)

temp_client:
	@echo Building client: $@...
	$(CC) $(CFLAGS) -D_GNU_SOURCE=1 -Wno-unused-result -o $@$(EXE_SUFFIX) src/$@.c $^

clean:
	rm -f temp_client temp_server *.o *.exe

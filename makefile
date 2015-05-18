CC=gcc
LIBS=-lncurses -lpthread

ifneq ($(wildcard /usr/include/cups/cups.h),"")
CFLAGS+=-DLIBCUPS_PRESENT
LIBS+=-lcups
$(info libcups support enabled.)
endif

ifneq ($(wildcard /usr/include/cups/cups.h),"")
CFLAGS+=-DLIBARCHIVE_PRESENT
LIBS+=-larchive
$(info libarchive support enabled.)
endif

ifneq ($(wildcard /usr/include/libconfig.h),"")
CFLAGS+=-DLIBCONFIG_PRESENT
LIBS+=-lconfig
$(info libconfig support enabled.)
endif

all: ncursesFM clean

objects:
	cd src/; $(CC) -c *.c $(CFLAGS)

ncursesFM: objects
	cd src/; $(CC) -o ../ncursesFM *.o $(LIBS)

clean:
	cd src/; rm *.o

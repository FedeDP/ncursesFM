CC = gcc
LIBS =-lpthread -lmagic $(shell pkg-config --silence-errors --libs libarchive ncurses)
CFLAGS =
RM = rm
INSTALL = install -p
INSTALL_PROGRAM = $(INSTALL) -m755
INSTALL_DATA = $(INSTALL) -m644
INSTALL_DIR = $(INSTALL) -d
BINDIR = /usr/bin
CONFDIR = /etc/default
BINNAME = ncursesFM
CONFNAME = ncursesFM.conf

ifeq (,$(findstring $(MAKECMDGOALS),"clean install uninstall"))
LIBX11=$(shell pkg-config --silence-errors --libs x11)
LIBCONFIG=$(shell pkg-config --silence-errors --libs libconfig)
LIBSYSTEMD=$(shell pkg-config --silence-errors --libs libsystemd)

LIBS+=$(LIBX11) $(LIBCONFIG) $(LIBSYSTEMD)

ifneq ("$(LIBX11)","")
CFLAGS+=-DLIBX11_PRESENT
$(info libX11 support enabled.)
endif

ifneq ("$(wildcard /usr/include/cups/cups.h)","")
CFLAGS+=-DLIBCUPS_PRESENT
LIBS+=-lcups
$(info libcups support enabled.)
endif

ifneq ("$(LIBCONFIG)","")
CFLAGS+=-DLIBCONFIG_PRESENT
$(info libconfig support enabled.)
endif

# Udev support is useful only if libsystemd support is enabled!
ifneq ("$(LIBSYSTEMD)","")
CFLAGS+=-DSYSTEMD_PRESENT
$(info libsystemd support enabled.)
LIBUDEV=$(shell pkg-config --silence-errors --libs libudev)
ifneq ("$(LIBUDEV)","")
CFLAGS+=-DLIBUDEV_PRESENT
LIBS+=$(LIBUDEV)
$(info libudev support enabled.)
endif
endif
endif

all: ncursesFM clean

objects:
	cd src/; $(CC) -c *.c -Wall $(CFLAGS)

ncursesFM: objects
	cd src/; $(CC) -o ../ncursesFM *.o $(LIBS)

clean:
	cd src/; $(RM) *.o

install:
	$(INSTALL_DIR) "$(DESTDIR)$(BINDIR)"
	$(INSTALL_PROGRAM) $(BINNAME) "$(DESTDIR)$(BINDIR)"
	$(INSTALL_DIR) "$(DESTDIR)$(CONFDIR)"
	$(INSTALL_DATA) $(CONFNAME) "$(DESTDIR)$(CONFDIR)"

uninstall:
	$(RM) "$(DESTDIR)$(BINDIR)/$(BINNAME)"
	$(RM) "$(DESTDIR)$(CONFDIR)/$(CONFNAME)"

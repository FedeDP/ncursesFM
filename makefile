CC = gcc
LIBS =-lpthread $(shell pkg-config --silence-errors --libs libarchive ncurses)
CFLAGS =-D_GNU_SOURCE
RM = rm
INSTALL = install -p
INSTALL_PROGRAM = $(INSTALL) -m755
INSTALL_DATA = $(INSTALL) -m644
INSTALL_DIR = $(INSTALL) -d
BINDIR = /usr/bin
CONFDIR = /etc/default
BINNAME = ncursesFM
CONFNAME = ncursesFM.conf
SRCDIR = src/

ifeq (,$(findstring $(MAKECMDGOALS),"clean install uninstall"))

LIBX11=$(shell pkg-config --silence-errors --libs x11)
LIBCONFIG=$(shell pkg-config --silence-errors --libs libconfig)
LIBSYSTEMD=$(shell pkg-config --silence-errors --libs libsystemd)
LIBGIT2=$(shell pkg-config --silence-errors --libs libgit2)

LIBS+=$(LIBX11) $(LIBCONFIG) $(LIBSYSTEMD) $(LIBGIT2)

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

ifneq ("$(LIBGIT2)","")
CFLAGS+=-DLIBGIT2_PRESENT
$(info libgit2 support enabled.)
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
	cd $(SRCDIR); $(CC) -c *.c -Wall $(CFLAGS)

ncursesFM: objects
	cd $(SRCDIR); $(CC) -o ../ncursesFM *.o $(LIBS)

clean:
	cd $(SRCDIR); $(RM) *.o

install:
	$(INSTALL_DIR) "$(DESTDIR)$(BINDIR)"
	$(INSTALL_PROGRAM) $(BINNAME) "$(DESTDIR)$(BINDIR)"
	$(INSTALL_DIR) "$(DESTDIR)$(CONFDIR)"
	$(INSTALL_DATA) $(CONFNAME) "$(DESTDIR)$(CONFDIR)"

uninstall:
	$(RM) "$(DESTDIR)$(BINDIR)/$(BINNAME)"
	$(RM) "$(DESTDIR)$(CONFDIR)/$(CONFNAME)"

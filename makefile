CC = gcc
LIBS = -lncurses -lpthread -larchive -lmagic
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
ifneq ("$(wildcard /usr/include/X11/Xlib.h)","")
CFLAGS+=-DLIBX11_PRESENT
LIBS+=-lX11
$(info libX11 support enabled.)
endif

ifneq ("$(wildcard /usr/include/cups/cups.h)","")
CFLAGS+=-DLIBCUPS_PRESENT
LIBS+=-lcups
$(info libcups support enabled.)
endif

ifneq ("$(wildcard /usr/include/libconfig.h)","")
CFLAGS+=-DLIBCONFIG_PRESENT
LIBS+=-lconfig
$(info libconfig support enabled.)
endif

ifneq ("$(wildcard /proc/1/comm)","systemd")
CFLAGS+=-DSYSTEMD_PRESENT
LIBS+=-lsystemd
$(info logind support enabled.)
endif

ifneq ("$(wildcard /usr/include/libudev.h)","")
CFLAGS+=-DLIBUDEV_PRESENT
LIBS+=-ludev
$(info libudev support enabled.)
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

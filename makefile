LIBS =-lpthread $(shell pkg-config --libs libarchive ncursesw libudev)
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
COMPLNAME = ncursesfm
SRCDIR = src/
COMPLDIR = $(shell pkg-config --variable=completionsdir bash-completion)

# sanity checks for completion dir
ifeq ("$(COMPLDIR)","")
COMPLDIR = $(shell pkg-config --variable=compatdir bash-completion)
ifeq ("$(COMPLDIR)","")
COMPLDIR = /etc/bash_completion.d
endif
endif

ifeq (,$(findstring $(MAKECMDGOALS),"clean install uninstall"))

ifneq ("$(DISABLE_LIBX11)","1")
LIBX11=$(shell pkg-config --silence-errors --libs x11)
endif
ifneq ("$(DISABLE_LIBCONFIG)","1")
LIBCONFIG=$(shell pkg-config --silence-errors --libs libconfig)
endif
ifneq ("$(DISABLE_LIBSYSTEMD)","1")
LIBSYSTEMD=$(shell pkg-config --silence-errors --libs libsystemd)
endif

LIBS+=$(LIBX11) $(LIBCONFIG) $(LIBSYSTEMD)

ifneq ("$(LIBX11)","")
CFLAGS+=-DLIBX11_PRESENT
$(info libX11 support enabled.)
endif

ifneq ("$(DISABLE_LIBCUPS)","1")
ifneq ("$(wildcard /usr/include/cups/cups.h)","")
CFLAGS+=-DLIBCUPS_PRESENT
LIBS+=-lcups
$(info libcups support enabled.)
endif
endif

ifneq ("$(LIBCONFIG)","")
CFLAGS+=-DLIBCONFIG_PRESENT -DCONFDIR=\"$(CONFDIR)\"
$(info libconfig support enabled.)
endif

ifneq ("$(LIBSYSTEMD)","")
CFLAGS+=-DSYSTEMD_PRESENT
$(info libsystemd support enabled.)
endif

endif

all: ncursesFM clean

debug: ncursesFM-debug clean

objects:
	cd $(SRCDIR); $(CC) -c *.c $(CFLAGS) -std=c99

objects-debug:
	cd $(SRCDIR); $(CC) -c *.c -Wall $(CFLAGS) -std=c99 -Werror -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wformat -Wformat-security -g

ncursesFM: objects
	cd $(SRCDIR); $(CC) -o ../ncursesFM *.o $(LIBS)

ncursesFM-debug: objects-debug
	cd $(SRCDIR); $(CC) -o ../ncursesFM *.o $(LIBS)

clean:
	cd $(SRCDIR); $(RM) *.o

install:
# 	install executable file
	$(INSTALL_DIR) "$(DESTDIR)$(BINDIR)"
	$(INSTALL_PROGRAM) $(BINNAME) "$(DESTDIR)$(BINDIR)"
# 	install conf file
	$(INSTALL_DIR) "$(DESTDIR)$(CONFDIR)"
	$(INSTALL_DATA) $(CONFNAME) "$(DESTDIR)$(CONFDIR)"
# 	install bash autocompletion script
	$(INSTALL_DIR) "$(DESTDIR)$(COMPLDIR)"
	$(INSTALL_DATA) $(COMPLNAME) "$(DESTDIR)$(COMPLDIR)/$(BINNAME)"

uninstall:
# 	remove executable file
	$(RM) "$(DESTDIR)$(BINDIR)/$(BINNAME)"
# 	remove conf file
	$(RM) "$(DESTDIR)$(CONFDIR)/$(CONFNAME)"
# 	remove bash autocompletion script
	$(RM) "$(DESTDIR)$(COMPLDIR)/$(BINNAME)"

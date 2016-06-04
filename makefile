SHELL = /bin/bash
RM = rm -f
INSTALL = install -p
INSTALL_PROGRAM = $(INSTALL) -m755
INSTALL_DATA = $(INSTALL) -m644
INSTALL_DIR = $(INSTALL) -d
GIT = git
BINDIR = /usr/bin
CONFDIR = /etc/default
LOCALEDIR = /usr/share/locale
BINNAME = ncursesFM
CONFNAME = ncursesFM.conf
COMPLNAME = ncursesFM
SCRIPTDIR = Script
PREVIEWERNAME = ncursesfm_previewer
SRCDIR = src/
COMPLDIR = $(shell pkg-config --variable=completionsdir bash-completion)
MSGLANGS = $(notdir $(wildcard msg/*po))
MSGOBJS = $(MSGLANGS:.po=/LC_MESSAGES/ncursesFM.mo)
MSGFILES = $(LOCALEDIR)/$(MSGOBJS)
LIBS =-lpthread $(shell pkg-config --libs libarchive ncursesw libudev)
CFLAGS =-D_GNU_SOURCE $(shell pkg-config --cflags libarchive ncursesw libudev) -DCONFDIR=\"$(CONFDIR)\" -DBINDIR=\"$(BINDIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

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
SYSTEMD_VERSION=$(shell pkg-config --modversion --silence-errors systemd)
ifeq ($(shell test $(SYSTEMD_VERSION) -ge 221; echo $$?), 0)
LIBSYSTEMD=$(shell pkg-config --silence-errors --libs libsystemd)
else
$(info systemd support disabled, minimum required version 221.)
endif
endif

LIBS+=$(LIBX11) $(LIBCONFIG) $(LIBSYSTEMD)

ifneq ("$(LIBX11)","")
CFLAGS+=-DLIBX11_PRESENT $(shell pkg-config --silence-errors --cflags libx11)
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
CFLAGS+=-DLIBCONFIG_PRESENT $(shell pkg-config --silence-errors --cflags libconfig)
$(info libconfig support enabled.)
endif

ifneq ("$(LIBSYSTEMD)","")
CFLAGS+=-DSYSTEMD_PRESENT $(shell pkg-config --silence-errors --cflags libsystemd)
$(info libsystemd support enabled.)
endif

endif

NCURSESFM_VERSION = $(shell git describe --abbrev=0 --always --tags)
CFLAGS+=-DVERSION=\"$(NCURSESFM_VERSION)\"

all: version ncursesFM clean gettext

debug: version ncursesFM-debug gettext

local: LOCALEDIR=$(shell pwd)/$(SCRIPTDIR)
local: BINDIR=$(shell pwd)/$(SCRIPTDIR)
local: CONFDIR=$(shell pwd)/$(SCRIPTDIR)
local: all

version:
	@$(GIT) rev-parse HEAD | awk ' BEGIN {print "#include \"../inc/version.h\""} {print "const char *build_git_sha = \"" $$0"\";"} END {}' > src/version.c
	@date | awk 'BEGIN {} {print "const char *build_git_time = \""$$0"\";"} END {} ' >> src/version.c

objects:
	@cd $(SRCDIR); $(CC) -c *.c $(CFLAGS) -std=c99

objects-debug:
	@cd $(SRCDIR); $(CC) -c *.c -Wall $(CFLAGS) -std=c99 -Werror -Wshadow -Wstrict-overflow -fno-strict-aliasing -Wformat -Wformat-security -g

ncursesFM: objects
	@cd $(SRCDIR); $(CC) -o ../ncursesFM *.o $(LIBS)

ncursesFM-debug: objects-debug
	@cd $(SRCDIR); $(CC) -o ../ncursesFM *.o $(LIBS)

clean:
	@cd $(SRCDIR); $(RM) *.o

gettext: $(MSGOBJS)

%/LC_MESSAGES/ncursesFM.mo: msg/%.po
	@mkdir -p $(dir $@)
	$(info building locales.)
	@msgfmt -c -o $@ msg/$*.po

install:
# 	install executable file
	$(info installing bin.)
	@$(INSTALL_DIR) "$(DESTDIR)$(BINDIR)"
	@$(INSTALL_PROGRAM) $(BINNAME) "$(DESTDIR)$(BINDIR)"
# 	install conf file
	$(info installing conf file.)
	@$(INSTALL_DIR) "$(DESTDIR)$(CONFDIR)"
	@$(INSTALL_DATA) $(SCRIPTDIR)/$(CONFNAME) "$(DESTDIR)$(CONFDIR)"
# 	install bash autocompletion script
	$(info installing bash autocompletion.)
	@$(INSTALL_DIR) "$(DESTDIR)$(COMPLDIR)"
	@$(INSTALL_DATA) $(SCRIPTDIR)/$(COMPLNAME) "$(DESTDIR)$(COMPLDIR)/$(COMPLNAME)"
# 	install image previewing script
	$(info installing img previewer.)
	@$(INSTALL_DIR) "$(DESTDIR)$(BINDIR)"
	@$(INSTALL_PROGRAM) $(SCRIPTDIR)/$(PREVIEWERNAME) "$(DESTDIR)$(BINDIR)"
#	install locales
	$(info installing locales.)
	@for MSGOBJ in $(MSGOBJS) ; do \
		MSGDIR=$(LOCALEDIR)/$$(dirname $$MSGOBJ) ;\
		$(INSTALL_DIR) "$(DESTDIR)$$MSGDIR" ; \
		$(INSTALL_DATA) $$MSGOBJ "$(DESTDIR)$$MSGDIR" ; \
	done

uninstall:
# 	remove executable file
	$(info uninstalling bin.)
	@$(RM) "$(DESTDIR)$(BINDIR)/$(BINNAME)"
# 	remove conf file
	$(info uninstalling conf file.)
	@$(RM) "$(DESTDIR)$(CONFDIR)/$(CONFNAME)"
# 	remove bash autocompletion script
	$(info uninstalling bash autocompletion.)
	@$(RM) "$(DESTDIR)$(COMPLDIR)/$(BINNAME)"
# 	remove image previewing script
	$(info uninstalling img previewer.)
	@$(RM) "$(DESTDIR)$(BINDIR)/$(PREVIEWERNAME)"
# 	remove locales
	$(info uninstalling locales.)
	@$(RM) "$(DESTDIR)$(MSGFILES)"

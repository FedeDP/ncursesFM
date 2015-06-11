CC=gcc
LIBS=-lncurses -lpthread -lcups -larchive -lconfig -lcrypto -lmagic -lX11
RM = rm
INSTALL = install -p
INSTALL_PROGRAM = $(INSTALL) -m755
INSTALL_DATA = $(INSTALL) -m644
INSTALL_DIR = $(INSTALL) -d
BINDIR = /usr/bin
CONFDIR = /etc/default
BINNAME = ncursesFM
CONFNAME = ncursesFM.conf

all: ncursesFM clean

objects:
	cd src/; $(CC) -c *.c -Wall

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
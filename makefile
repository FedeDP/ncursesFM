CC=gcc
LIBS=-lncurses -lpthread -lcups -larchive -lconfig -lcrypto -lmagic -lX11

all: ncursesFM clean

objects:
	cd src/; $(CC) -c *.c -Wall

ncursesFM: objects
	cd src/; $(CC) -o ../ncursesFM *.o $(LIBS)

clean:
	cd src/; rm *.o

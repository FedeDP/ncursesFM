CC=gcc
LIBS=-lncurses -lpthread -lcups -larchive -lconfig

all: ncursesFM clean

objects:
	cd src/; $(CC) -c *.c

ncursesFM: objects
	cd src/; $(CC) -o ../ncursesFM *.o $(LIBS)

clean:
	cd src/; rm *.o

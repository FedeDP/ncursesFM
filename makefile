all: ncursesFM clean

objects:
	cd src/; gcc -c *.c

ncursesFM: objects
	cd src/; gcc -o ../ncursesFM *.o -lncurses -lpthread -lconfig

clean:
	cd src/; rm *.o

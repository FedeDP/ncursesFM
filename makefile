all: ncursesFM clean

ncursesFM:
	cd src/; gcc -c *.c
	cd src/; gcc -o ../ncursesFM *.o -lncurses -lpthread -lconfig

clean:
	cd src/; rm *.o

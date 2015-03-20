all:
	cd src/; gcc -c *.c
	cd src/; gcc -o ../ncursesFM *.o -lncurses -lpthread -lconfig

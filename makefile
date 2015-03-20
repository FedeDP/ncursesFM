all:
	cd src/; gcc -o ncursesFM main.c quit_functions.c helper_functions.c ui_functions.c fm_functions.c -lncurses -lpthread -lconfig

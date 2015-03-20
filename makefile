all:
	gcc -o ncursesfm main.c quit_functions.c helper_functions.c ui_functions.c fm_functions.c -lncurses -lpthread -lconfig

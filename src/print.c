#ifdef LIBCUPS_PRESENT

#include "../inc/print.h"

static void *print_file(void *filename);

void print_support(char *str) {
    pthread_t print_thread;
    pthread_attr_t tattr;
    char c;

    ask_user(print_question, &c, 1, 'y');
    if (!quit && c == 'y') {
        pthread_attr_init(&tattr);
        pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
        pthread_create(&print_thread, &tattr, print_file, str);
        pthread_attr_destroy(&tattr);
    }
}

/*
 * If there are cups printers available, take the default printer and print the file.
 */
static void *print_file(void *filename) {
    cups_dest_t *dests, *default_dest;
    int num_dests = cupsGetDests(&dests);
    int r;

    if (num_dests > 0) {
        default_dest = cupsGetDest(NULL, NULL, num_dests, dests);
        r = cupsPrintFile(default_dest->name, (char *)filename, "ncursesFM job",
                      default_dest->num_options, default_dest->options);
        if (r) {
            print_info(print_ok, INFO_LINE);
        } else {
            print_info(ippErrorString(cupsLastError()), ERR_LINE);
        }
    } else {
        print_info(print_fail, ERR_LINE);
    }
    pthread_exit(NULL);
}

#endif
#ifdef LIBCUPS_PRESENT

#include "../inc/print.h"

static void *print_file(void *filename);

void print_support(char *str) {
    pthread_t print_thread;
    char c;

    ask_user(print_question, &c, 1, 'y');
    if (c == 'y') {
        pthread_create(&print_thread, NULL, print_file, str);
        pthread_detach(print_thread);
    }
}

static void *print_file(void *filename) {
    cups_dest_t *dests, *default_dest;
    int num_dests = cupsGetDests(&dests);

    if (num_dests > 0) {
        default_dest = cupsGetDest(NULL, NULL, num_dests, dests);
        cupsPrintFile(default_dest->name, (char *)filename, "ncursesFM job", default_dest->num_options, default_dest->options);
        print_info(print_ok, INFO_LINE);
    } else {
        print_info(print_fail, ERR_LINE);
    }
    return NULL;
}

#endif
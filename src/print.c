#ifdef LIBCUPS_PRESENT

#include "../inc/print.h"

static void print_file(const char *filename);

void print_support(const char *str) {
    char c;

    if (config.safe != UNSAFE) {
        ask_user(_(print_question), &c, 1);
        if (c != _(yes)[0]) {
            return;
        }
    }
    print_file(str);
}

/*
 * If there are cups printers available, take the default printer and print the file.
 */
static void print_file(const char *filename) {
    cups_dest_t *dests;
#ifdef LIBNOTIFY_PRESENT
    const char *str;
#endif
    int num_dests = cupsGetDests(&dests);

    if (num_dests > 0) {
        cups_dest_t *default_dest = cupsGetDest(NULL, NULL, num_dests, dests);
        int r = cupsPrintFile(default_dest->name, filename, "ncursesFM job",
                              default_dest->num_options, default_dest->options);
        if (r) {
            print_info(_(print_ok), INFO_LINE);
#ifdef LIBNOTIFY_PRESENT
            str = _(print_ok);
#endif
        } else {
            print_info(ippErrorString(cupsLastError()), ERR_LINE);
#ifdef LIBNOTIFY_PRESENT
            str = ippErrorString(cupsLastError());
#endif
        }
    } else {
        print_info(_(print_fail), ERR_LINE);
#ifdef LIBNOTIFY_PRESENT
        str = _(print_fail);
#endif
    }

#ifdef LIBNOTIFY_PRESENT
    send_notification(str);
#endif
}

#endif

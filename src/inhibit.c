#ifdef SYSTEMD_PRESENT

#include "../inc/inhibit.h"

static sd_bus *bus;
static sd_bus_message *m;

void inhibit_suspend(void) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int r = sd_bus_open_system(&bus);
    if (r < 0) {
        print_info(bus_error, ERR_LINE);
        return;
    }
    r = sd_bus_call_method(bus,
                           "org.freedesktop.login1",
                           "/org/freedesktop/login1",
                           "org.freedesktop.login1.Manager",
                           "Inhibit",
                            &error,
                            &m,
                           "ssss",
                           "sleep:idle",
                           "ncursesFM",
                           "Job in process...",
                           "block");
    if (r < 0) {
        print_info(error.message, ERR_LINE);
    } else {
        r = sd_bus_message_read(m, "h", NULL);
        if (r < 0) {
            print_info(strerror(-r), ERR_LINE);
        }
    }
    sd_bus_error_free(&error);
}

void free_bus(void) {
    if (m) {
        sd_bus_message_unref(m);
        m = NULL;
    }
    if (bus) {
        sd_bus_flush_close_unref(bus);
        bus = NULL;
    }
}

#endif
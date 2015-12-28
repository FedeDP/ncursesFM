#ifdef SYSTEMD_PRESENT

#include "../inc/inhibit.h"


/*
 * Thanks elogind project for some hints to improve my old implementation.
 * https://github.com/andywingo/elogind/blob/master/src/login/inhibit.c
 */
int inhibit_suspend(const char *str) {
    sd_bus *bus;
    sd_bus_message *reply = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int r = sd_bus_open_system(&bus);
    int fd;
    
    if (r < 0) {
        print_info(bus_error, ERR_LINE);
        WARN(bus_error);
        return r;
    }
    INFO("calling Inhibit method on bus.");
    r = sd_bus_call_method(bus,
                           "org.freedesktop.login1",
                           "/org/freedesktop/login1",
                           "org.freedesktop.login1.Manager",
                           "Inhibit",
                            &error,
                           &reply,
                           "ssss",
                           "sleep:idle",
                           "ncursesFM",
                           str,
                           "block");
    if (r < 0) {
        print_info(error.message, ERR_LINE);
        WARN(bus_call_fail);
    } else {
        r = sd_bus_message_read_basic(reply, SD_BUS_TYPE_UNIX_FD, &fd);
        if (r < 0) {
            print_info(strerror(-r), ERR_LINE);
        } else {
            r = fcntl(fd, F_DUPFD, 3);
            INFO("power management functions inhibition started.");
        }
    }
    close_bus(&error, reply, bus);
    return r;
}

void close_bus(sd_bus_error *error, sd_bus_message *mess, sd_bus *bus) {
    if (mess) {
        sd_bus_message_unref(mess);
    }
    if (error) {
        sd_bus_error_free(error);
    }
    if (bus) {
        sd_bus_flush_close_unref(bus);
    }
}

#endif
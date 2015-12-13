#ifdef SYSTEMD_PRESENT

#include "../inc/install_package.h"

static int match_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static void close_bus(sd_bus_error *error, sd_bus_message *mess, sd_bus *bus);

/*
 * First of all, creates a transaction;
 * then reads the newly created bus path,
 * adds a match to the bus to wait until installation is really finished before notifying the user.
 * finally, calls InstallFiles method, and processes the bus while finished == 0.
 */
void *install_package(void *str) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *mess = NULL;
    sd_bus *install_bus = NULL;
    const char *path;
    int r, finished = 0;

    r = sd_bus_open_system(&install_bus);
    if (r < 0) {
        print_info(bus_error, ERR_LINE);
        pthread_exit(NULL);
    }
    r = sd_bus_call_method(install_bus,
                           "org.freedesktop.PackageKit",
                           "/org/freedesktop/PackageKit",
                           "org.freedesktop.PackageKit",
                           "CreateTransaction",
                           &error,
                           &mess,
                           NULL);
    if (r < 0) {
        print_info(error.message, ERR_LINE);
        close_bus(&error, mess, install_bus);
        pthread_exit(NULL);
    }
    sd_bus_message_read(mess, "o", &path);
    r = sd_bus_add_match(install_bus, NULL, "type='signal',interface='org.freedesktop.PackageKit.Transaction',member='Finished'", match_callback, &finished);
    if (r < 0) {
        print_info(strerror(-r), ERR_LINE);
    } else {
        sd_bus_flush(install_bus);
        r = sd_bus_call_method(install_bus,
                           "org.freedesktop.PackageKit",
                           path,
                           "org.freedesktop.PackageKit.Transaction",
                           "InstallFiles",
                           &error,
                           NULL,
                           "tas",
                           0,
                           1,
                           (char *)str);
        if (r < 0) {
            print_info(error.message, ERR_LINE);
        } else {
            while (!finished) {
                r = sd_bus_process(install_bus, NULL);
                if (r > 0) {
                    continue;
                }
                r = sd_bus_wait(install_bus, (uint64_t) -1);
                if (r < 0) {
                    break;
                }
            }
        }
    }
    close_bus(&error, mess, install_bus);
    pthread_exit(NULL);
}

static int match_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    const char *success = "Installed.", *install_failed = "Could not install.";
    unsigned int ret;

    *(int *)userdata = 1;
    sd_bus_message_read(m, "u", &ret);
    if (ret == 1) {
        print_info(success, INFO_LINE);
    } else {
        print_info(install_failed, ERR_LINE);
    }
    return 0;
}

static void close_bus(sd_bus_error *error, sd_bus_message *mess, sd_bus *bus) {
    sd_bus_message_unref(mess);
    sd_bus_error_free(error);
    sd_bus_flush_close_unref(bus);
}

#endif
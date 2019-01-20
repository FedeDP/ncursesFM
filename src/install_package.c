#include "../inc/install_package.h"

static int match_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

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
    int r, inhibit_fd = -1, finished = 0;
    
    r = sd_bus_open_system(&install_bus);
    if (r < 0) {
        print_and_warn(strerror(-r), ERR_LINE);
        goto finish;
    }
    INFO("calling CreateTransaction on bus.");
    r = sd_bus_call_method(install_bus,
                           "org.freedesktop.PackageKit",
                           "/org/freedesktop/PackageKit",
                           "org.freedesktop.PackageKit",
                           "CreateTransaction",
                           &error,
                           &mess,
                           NULL);
    if (r < 0) {
        print_and_warn(error.message, ERR_LINE);
        goto finish;
    }
    if (config.inhibit) {
        inhibit_fd = inhibit_suspend("Package installation...");
    }
    sd_bus_message_read(mess, "o", &path);
    r = sd_bus_add_match(install_bus, NULL, "type='signal',interface='org.freedesktop.PackageKit.Transaction',member='Finished'", match_callback, &finished);
    if (r < 0) {
        print_and_warn(strerror(-r), ERR_LINE);
        goto finish;
    }
    sd_bus_flush(install_bus);
    INFO("calling InstallFiles on bus.");
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
        print_and_warn(error.message, ERR_LINE);
        goto finish;
    }
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

finish:
    if (config.inhibit) {
        stop_inhibition(inhibit_fd);
    }
    close_bus(&error, mess, install_bus);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

static int match_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    unsigned int ret;
#ifdef LIBNOTIFY_PRESENT
    char *str;
#endif
    
    *(int *)userdata = 1;
    sd_bus_message_read(m, "u", &ret);
    if (ret == 1) {
        print_info(_(install_success), INFO_LINE);
#ifdef LIBNOTIFY_PRESENT
        str = _(install_success);
#endif
    } else {
        print_info(_(install_failed), ERR_LINE);
#ifdef LIBNOTIFY_PRESENT
        str = _(install_failed);
#endif
    }
#ifdef LIBNOTIFY_PRESENT
    send_notification(str);
#endif
    return 0;
}

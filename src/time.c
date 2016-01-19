#include "../inc/time.h"

#ifdef LIBUDEV_PRESENT
static void poll_batteries(void);
#endif

static int timerfd;
#ifdef LIBUDEV_PRESENT
static int num_of_batt;
static char ac_path[PATH_MAX + 1];
static struct supply *batt;
static struct udev *udev;
#endif

/*
 * Create 30s timer and returns its fd to
 * the main struct pollfd
 */
int start_timer(void) {
    struct itimerspec timerValue = {0};
    
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd == -1) {
        WARN("could not start timer.");
    } else {
#ifdef LIBUDEV_PRESENT
        udev = udev_new();
        poll_batteries();
#endif
        timerValue.it_value.tv_sec = 30;
        timerValue.it_value.tv_nsec = 0;
        timerValue.it_interval.tv_sec = 30;
        timerValue.it_interval.tv_nsec = 0;
        timerfd_settime(timerfd, 0, &timerValue, NULL);
        INFO("started time/battery monitor.");
        timer_func();
    }
    return timerfd;
}

void timer_func(void) {
    update_time();

#ifdef LIBUDEV_PRESENT
    struct udev_device *dev;
    int perc[num_of_batt];
    char name[num_of_batt][NAME_MAX + 1];

    dev = udev_device_new_from_syspath(udev, ac_path);
    int online = strtol(udev_device_get_property_value(dev, "POWER_SUPPLY_ONLINE"), NULL, 10);
    udev_device_unref(dev);
    if (!online) {
        for (int i = 0; i < num_of_batt; i++) {
            dev = udev_device_new_from_syspath(udev, batt[i].path);
            int energy_now = strtol(udev_device_get_property_value(dev, "POWER_SUPPLY_ENERGY_NOW"), NULL, 10);
            perc[i] = (float)energy_now / (float)batt[i].energy_full * 100;
            strcpy(name[i], udev_device_get_property_value(dev, "POWER_SUPPLY_NAME"));
            udev_device_unref(dev);
        }
    }
    update_batt(online, perc, num_of_batt, name);
#else
    update_batt(-1, NULL, 0, NULL);
#endif
}

void free_timer(void) {
    close(timerfd);
#ifdef LIBUDEV_PRESENT
    if (batt) {
        free(batt);
    }
    udev_unref(udev);
#endif
}

#ifdef LIBUDEV_PRESENT
static void poll_batteries(void) {
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;
    
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "power_supply");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);
        if (udev_device_get_property_value(dev, "POWER_SUPPLY_ONLINE")) {
            strcpy(ac_path, path);
        } else {
            batt = realloc(batt, sizeof(struct supply) * (num_of_batt + 1));
            if (batt) {
                strcpy(batt[num_of_batt].path, path);
                int energy = strtol(udev_device_get_property_value(dev, "POWER_SUPPLY_ENERGY_FULL"), NULL, 10);
                batt[num_of_batt].energy_full = energy;
                num_of_batt++;
            }
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
}
#endif
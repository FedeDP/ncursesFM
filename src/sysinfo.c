#include "../inc/sysinfo.h"

static void update_battery(int where);
static void poll_batteries(void);

static int timerfd;
static int num_of_batt;
static char ac_path[PATH_MAX + 1], (*batt)[PATH_MAX + 1];
static struct udev *udev;

/*
 * Create 30s timer and returns its fd to
 * the main struct pollfd
 */
int start_timer(void) {
    struct itimerspec timerValue = {{0}};
    
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd == -1) {
        WARN("could not start timer.");
    } else {
        udev = udev_new();
        poll_batteries();
        timerValue.it_value.tv_sec = 30;
        timerValue.it_value.tv_nsec = 0;
        timerValue.it_interval.tv_sec = 30;
        timerValue.it_interval.tv_nsec = 0;
        timerfd_settime(timerfd, 0, &timerValue, NULL);
        INFO("started time/battery monitor.");
    }
    return timerfd;
}

/*
 * Called from main_poll();
 * it checks power_supply online status, and if !online,
 * checks batteries perc level.
 * Then calls update_batt (ui_functions)
 */
void timer_func(void) {
    for (int i = 0; i < strlen(config.sysinfo_layout); i++) {
        switch (tolower(config.sysinfo_layout[i])) {
        case 'c':
            update_time(i);
            break;
        case 'p':
            update_sysinfo(i);
            break;
        case 'b':
            update_battery(i);
            break;
        default:
            break;
        }
    }
}

static void update_battery(int where) {
    struct udev_device *dev;
    int perc[num_of_batt], online;
    char name[num_of_batt][10];

    dev = udev_device_new_from_syspath(udev, ac_path);
    if (dev) {
        online = strtol(udev_device_get_property_value(dev, "POWER_SUPPLY_ONLINE"), NULL, 10);
        udev_device_unref(dev);
        if (!online) {
            for (int i = 0; i < num_of_batt; i++) {
                dev = udev_device_new_from_syspath(udev, batt[i]);
                if (dev && udev_device_get_property_value(dev, "POWER_SUPPLY_CAPACITY")) {
                    perc[i] = strtol(udev_device_get_property_value(dev, "POWER_SUPPLY_CAPACITY"), NULL, 10);
                } else {
                    perc[i] = -1;
                }
                if (dev && udev_device_get_property_value(dev, "POWER_SUPPLY_NAME")) {
                    strcpy(name[i], udev_device_get_property_value(dev, "POWER_SUPPLY_NAME"));
                } else {
                    strcpy(name[i], "BAT");
                }
                udev_device_unref(dev);
            }
        }
    } else {
        online = -1;
    }
    update_batt(online, perc, num_of_batt, name, where);
}

void free_timer(void) {
    close(timerfd);
    if (batt) {
        free(batt);
    }
    udev_unref(udev);
}

/*
 * Initial battery polling: for each system battery,
 * save its path in batt[].
 * If property "POWER_SUPPLY_ONLINE" is present, then
 * current udev device is ac adapter.
 */
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
            strncpy(ac_path, path, PATH_MAX);
        } else {
            batt = realloc(batt, (PATH_MAX + 1) * (num_of_batt + 1));
            if (batt) {
                strncpy(batt[num_of_batt], path, PATH_MAX);
                num_of_batt++;
            }
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
}

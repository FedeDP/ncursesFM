#include "../inc/mount.h"

#ifdef LIBUDEV_PRESENT
static void enumerate_block_devices(void);
static int is_mounted(const char *dev_path);
static void *device_monitor(void *x);
#ifdef SYSTEMD_PRESENT
static void change_mounted_status(int pos, const char *name);
#endif
static void sig_handler(int signum);
static int add_device(struct udev_device *dev, const char *name);
static void remove_device(const char *name);
static void *safe_realloc(const size_t size);
static void free_devices(void);

static struct udev *udev;
struct udev_monitor *mon;
static int number_of_devices;
char (*my_devices)[PATH_MAX + 1];
#endif


#ifdef SYSTEMD_PRESENT
/*
 * Open the bus, and call "method" string on UDisks2.Filesystem.
 */
void mount_fs(const char *str, const char *method, int mount) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *mess = NULL;
    sd_bus *mount_bus = NULL;
    const char *path;
    char obj_path[PATH_MAX + 1] = "/org/freedesktop/UDisks2/block_devices/";
    char success[PATH_MAX + 1], mount_str[100];
    int r;

    r = sd_bus_open_system(&mount_bus);
    if (r < 0) {
        print_info(bus_error, ERR_LINE);
        WARN(bus_error);
        return;
    }
    strcat(obj_path, strrchr(str, '/') + 1);
    sprintf(mount_str, "calling %s on bus.", method);
    INFO(mount_str);
    r = sd_bus_call_method(mount_bus,
                           "org.freedesktop.UDisks2",
                           obj_path,
                           "org.freedesktop.UDisks2.Filesystem",
                           method,
                           &error,
                           &mess,
                           "a{sv}",
                           NULL);
    if (r < 0) {
        print_info(error.message, ERR_LINE);
        WARN(bus_call_fail);
    } else {
        if (!mount) {
            sd_bus_message_read(mess, "s", &path);
            sprintf(success, "%s mounted in: %s.", str, path);
            print_info(success, INFO_LINE);
        } else {
            print_info("Unmounted.", INFO_LINE);
        }
    }
    close_bus(&error, mess, mount_bus);
}

/*
 * Open the bus and setup the loop device,
 * then set "SetAutoclear" option; this way the loop device
 * will be automagically cleared when no more in use.
 */
void isomount(const char *str) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *mess = NULL;
    sd_bus *iso_bus = NULL;
    const char *obj_path;
    int r, fd;

    fd = open(str, O_RDONLY);
    r = sd_bus_open_system(&iso_bus);
    if (r < 0) {
        print_info(bus_error, ERR_LINE);
        WARN(bus_error);
        return;
    }
    INFO("calling LoopSetup method on bus.");
    r = sd_bus_call_method(iso_bus,
                           "org.freedesktop.UDisks2",
                           "/org/freedesktop/UDisks2/Manager",
                           "org.freedesktop.UDisks2.Manager",
                           "LoopSetup",
                           &error,
                           &mess,
                           "ha{sv}",
                           fd,
                           NULL);
    close(fd);
    if (r < 0) {
        print_info(error.message, ERR_LINE);
        WARN(bus_call_fail);
    } else {
        sd_bus_message_read(mess, "o", &obj_path);
        mount_fs(obj_path, "Mount", 0);
        sd_bus_flush(iso_bus);
        INFO("calling SetAutoClear on bus.");
        r = sd_bus_call_method(iso_bus,
                               "org.freedesktop.UDisks2",
                               obj_path,
                               "org.freedesktop.UDisks2.Loop",
                               "SetAutoclear",
                               &error,
                               NULL,
                               "ba{sv}",
                               TRUE,
                               NULL);
        if (r < 0) {
            print_info(error.message, ERR_LINE);
            WARN(bus_call_fail);
        }
    }
    close_bus(&error, mess, iso_bus);
}
#endif

#ifdef LIBUDEV_PRESENT
void start_udev(void) {
    udev = udev_new();
    if (!udev) {
        print_info("Couldn't create udev. You won't be able to use libudev functions.", ERR_LINE);
        WARN("could not create udev.");
        return;
    }
    enumerate_block_devices();
    if (!quit) {
        if (config.monitor) {
            INFO("started device monitor th.");
            pthread_create(&monitor_th, NULL, device_monitor, NULL);
        } else {
            udev_unref(udev);
        }
    } else {
        free_devices();
    }
}

void show_devices_tab(void) {
    if (number_of_devices) {
        pthread_mutex_lock(&fm_lock[active]);
        ps[active].number_of_files = number_of_devices;
        str_ptr[active] = my_devices;
        device_mode = 1 + active;
        sprintf(ps[active].title, device_mode_str);
        reset_win(active);
        pthread_mutex_unlock(&fm_lock[active]);
    } else {
        print_info("No devices found.", INFO_LINE);
    }
}

/*
 * Scan "block" subsystem for devices.
 * For each device check if it is a really mountable external fs,
 * add it to my_devices{} and increment number_of_devices.
 */
static void enumerate_block_devices(void) {
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);
        const char *name = udev_device_get_devnode(dev);
        add_device(dev, name);
        udev_device_unref(dev);
        if (quit) {
            break;
        }
    }
    udev_enumerate_unref(enumerate);
}

/*
 * Open mtab in read mode to check whether dev_path is mounted or not.
 */
static int is_mounted(const char *dev_path) {
    FILE *mtab = NULL;
    struct mntent *part = NULL;
    int is_mounted = 0;

    if ((mtab = setmntent ("/etc/mtab", "r"))) {
        while ((part = getmntent(mtab))) {
            if ((part->mnt_fsname) && (!strcmp(part->mnt_fsname, dev_path))) {
                is_mounted = 1;
                break;
            }
        }
        endmntent(mtab);
    }
    return is_mounted;
}

void manage_enter_device(void) {
#ifdef SYSTEMD_PRESENT
    int mount;
    int pos = ps[active].curr_pos;
    int len = strlen(my_devices[pos]);
    char *ptr = strchr(my_devices[pos], ',');
    char name[PATH_MAX + 1], action[10];

    mount = my_devices[pos][len - 1] - '0';
    strcpy(name, my_devices[pos]);
    name[len - strlen(ptr)] = '\0';
    if (mount) {
        strcpy(action, "Unmount");
    } else {
        strcpy(action, "Mount");
    }
    mount_fs(name, action, mount);
    change_mounted_status(pos, name);
    update_devices(number_of_devices, NULL);
#else
    print_info("You need sd-bus(systemd) for mounting support.", ERR_LINE);
#endif
}

#ifdef SYSTEMD_PRESENT
static void change_mounted_status(int pos, const char *name) {
    int len = strlen(my_devices[pos]);
    sprintf(my_devices[pos] + len - 1, "%d", is_mounted(name));
}
#endif

void leave_device_mode(void) {
    pthread_mutex_lock(&fm_lock[active]);
    if (!config.monitor) {
        free(my_devices);
        my_devices = NULL;
        number_of_devices = 0;
    }
    device_mode = 0;
    pthread_mutex_unlock(&fm_lock[active]);
    change_dir(ps[active].my_cwd);
}

/*
 * http://www.linuxprogrammingblog.com/code-examples/using-pselect-to-avoid-a-signal-race
 * thanks daper for the perfect pselect example + thanks udev crew for udev monitor example:
 * http://www.signal11.us/oss/udev/udev_example.c
 */
static void *device_monitor(void *x) {
    struct udev_device *dev;
    int fd, ret;
    fd_set fds;
    sigset_t mask;
    sigset_t orig_mask;
    struct sigaction act;
    
    memset(&act, 0, sizeof(act));
    act.sa_handler = sig_handler;
    sigaction(SIGUSR2, &act, 0);
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &orig_mask);

    mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
    udev_monitor_enable_receiving(mon);
    fd = udev_monitor_get_fd(mon);

    while (!quit) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        ret = pselect(fd + 1, &fds, NULL, NULL, NULL, &orig_mask);
        /* Check if our file descriptor has received data. */
        if (ret > 0 && FD_ISSET(fd, &fds)) {
            dev = udev_monitor_receive_device(mon);
            if (dev) {
                const char *name = udev_device_get_devnode(dev);
                if (strcmp(udev_device_get_action(dev), "add") == 0) {
                    if (add_device(dev, name) == 1) {
                        print_info("New device connected.", INFO_LINE);
#ifdef SYSTEMD_PRESENT
                        if (config.automount) {
                            mount_fs(name, "Mount", 0);
                            change_mounted_status(number_of_devices - 1, name);
                        }
                    }
#endif
                } else if (strcmp(udev_device_get_action(dev), "remove") == 0) {
                    remove_device(name);
                }
                udev_device_unref(dev);
                if (!quit && device_mode) {
                    update_devices(number_of_devices, my_devices);
                }
            }
        }
    }
    udev_monitor_unref(mon);
    free_devices();
    pthread_exit(NULL);
}

static void sig_handler(int signum) {
    INFO("received SIGUSR2 signal.");
}

static int add_device(struct udev_device *dev, const char *name) {
    long size;
    char s[20];
    int ret = 0;
    struct udev_device *usb_parent, *scsi_parent;
    
    /* if there's eg: sda1, do not print sda + do not list cd/dvd players */
    if (!udev_device_get_property_value(dev, "ID_FS_TYPE")) {
        return ret;
    }
    usb_parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    scsi_parent = udev_device_get_parent_with_subsystem_devtype(dev, "scsi", "scsi_device");
    if (usb_parent || scsi_parent) {
        my_devices = safe_realloc(PATH_MAX * (number_of_devices + 1));
        if (!quit) {
            /* calculates current device size */
            size = strtol(udev_device_get_sysattr_value(dev, "size"), NULL, 10);
            size = (size / (long) 2) * 1024;
            change_unit((float)size, s);
            if (usb_parent) {
                sprintf(my_devices[number_of_devices], "%s, %s %s, Size: %s, Mounted: %d", name,
                    udev_device_get_sysattr_value(usb_parent,"manufacturer"),
                    udev_device_get_sysattr_value(usb_parent,"product"), s, is_mounted(name));
            } else {
                sprintf(my_devices[number_of_devices], "%s, %s, Size: %s, Mounted: %d", name,
                    udev_device_get_sysattr_value(scsi_parent, "model"), s, is_mounted(name));
            }
            number_of_devices++;
            INFO("added device.");
            ret = 1;
        }
    }
    return ret;
}

static void remove_device(const char *name) {
    for (int i = 0; i < number_of_devices; i++) {
        if (strncmp(my_devices[i], name, strlen(name)) == 0) {
            memset(my_devices[i], 0, strlen(my_devices[i]));
            while (i < number_of_devices - 1) {
                strcpy(my_devices[i], my_devices[i + 1]);
                i++;
            }
            my_devices = safe_realloc(PATH_MAX * (number_of_devices - 1));
            if (!quit) {
                number_of_devices--;
                print_info("Device removed.", INFO_LINE);
                INFO("removed device.");
            }
            break;
        }
    }
}

static void *safe_realloc(const size_t size) {
    void *tmp = realloc(my_devices, size);
    
    if (!tmp) {
        quit = MEM_ERR_QUIT;
        ERROR("could not realloc. Leaving monitor th.");
        return my_devices;
    }
    return tmp;
}

static void free_devices(void) {
    INFO("freeing resources...");
    udev_unref(udev);
    if (my_devices) {
        free(my_devices);
    }
    INFO("freed.");
}

#endif
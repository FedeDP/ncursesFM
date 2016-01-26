#include "../inc/mount.h"

#ifdef SYSTEMD_PRESENT
static int mount_fs(const char *str, const char *method, int mount);

static char mount_str[PATH_MAX + 1];

#ifdef LIBUDEV_PRESENT
static int is_iso_mounted(const char *filename, char *loop_dev);
static void iso_backing_file(char *s, const char *name);
static int init_mbus(void);
static int check_udisks(void);
static int add_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int remove_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int change_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int change_power_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static void enumerate_block_devices(void);
static int is_mounted(const char *dev_path);
static void get_mount_point(const char *dev_path, char *path);
static int check_cwd(char *mounted_path);
static void change_mounted_status(int pos, const char *name);
static int add_device(struct udev_device *dev, const char *name);
static int remove_device(const char *name);
static int is_present(const char *name);
static void *safe_realloc(const size_t size);

static struct udev *udev;
static int number_of_devices;
char (*my_devices)[PATH_MAX + 1];
static sd_bus *bus;

#endif
#endif

#ifdef SYSTEMD_PRESENT
/*
 * Open the bus, and call "method" string on UDisks2.Filesystem.
 */
static int mount_fs(const char *str, const char *method, int mount) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *mess = NULL;
    sd_bus *mount_bus = NULL;
    const char *path;
    char obj_path[PATH_MAX + 1] = "/org/freedesktop/UDisks2/block_devices/";
    char tmp[30];
    int r, ret = -1;

    r = sd_bus_open_system(&mount_bus);
    if (r < 0) {
        print_and_warn(strerror(-r), ERR_LINE);
        goto finish;
    }
    strcat(obj_path, strrchr(str, '/') + 1);
    sprintf(tmp, "calling %s on bus.", method);
    INFO(tmp);
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
        print_and_warn(error.message, ERR_LINE);
        goto finish;
    }
    ret = 1;
    if (!mount) {
        sd_bus_message_read(mess, "s", &path);
        sprintf(mount_str, "%s mounted in: %s.", str, path);
        INFO("Mounted.");
    } else {
        sprintf(mount_str, "%s unmounted.", str);
        INFO("Unmounted.");
    }
#ifndef LIBUDEV_PRESENT
    print_info(mount_str, INFO_LINE);
#endif

finish:
    close_bus(&error, mess, mount_bus);
    return ret;
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

#ifdef LIBUDEV_PRESENT
    char loop_dev[PATH_MAX + 1];
    
    if (is_iso_mounted(str, loop_dev)) {
        mount_fs(loop_dev, "Unmount", 1);
        return;
    }
#endif
    fd = open(str, O_RDONLY);
    r = sd_bus_open_system(&iso_bus);
    if (r < 0) {
        print_and_warn(strerror(-r), ERR_LINE);
        goto finish;
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
        print_and_warn(error.message, ERR_LINE);
        goto finish;
    }
    sd_bus_message_read(mess, "o", &obj_path);
    sd_bus_flush(iso_bus);
#ifndef LIBUDEV_PRESENT
    mount_fs(obj_path, "Mount", 0);
#endif
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
        print_and_warn(error.message, ERR_LINE);
    }

finish:
    close_bus(&error, mess, iso_bus);
}

#ifdef LIBUDEV_PRESENT
/*
 * Check if iso is already mounted.
 * For each /dev/loop device present, calls iso_backing_file on bus.
 * As soon as it finds a matching backing_file with current file's filename,
 * it breaks the cycle and returns 1;
 */
static int is_iso_mounted(const char *filename, char loop_dev[PATH_MAX + 1]) {
    struct udev *tmp_udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;
    char s[PATH_MAX + 1] = {0};
    char resolved_path[PATH_MAX + 1];
    int mount = 0;

    realpath(filename, resolved_path);
    tmp_udev = udev_new();
    enumerate = udev_enumerate_new(tmp_udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_add_match_property(enumerate, "ID_FS_TYPE", "udf");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);
        strcpy(loop_dev, udev_device_get_devnode(dev));
        iso_backing_file(s, loop_dev);
        udev_device_unref(dev);
        if (strcmp(s, resolved_path) == 0) {
            mount = 1;
            break;
        }
    }
    udev_enumerate_unref(enumerate);
    udev_unref(tmp_udev);
    return mount;
}

/*
 * Given a loop device, returns the iso file mounted on it.
 */
static void iso_backing_file(char *s, const char *name) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *mess = NULL;
    sd_bus *iso_bus = NULL;
    int r;
    const uint8_t bytes;
    char obj_path[PATH_MAX + 1] = "/org/freedesktop/UDisks2/block_devices/";

    r = sd_bus_open_system(&iso_bus);
    if (r < 0) {
        print_and_warn(strerror(-r), ERR_LINE);
        goto finish;
    }
    strcat(obj_path, strrchr(name, '/') + 1);
    INFO("getting BackingFile property on bus.");
    r = sd_bus_get_property(iso_bus,
                            "org.freedesktop.UDisks2",
                            obj_path,
                            "org.freedesktop.UDisks2.Loop",
                            "BackingFile",
                            &error,
                            &mess,
                            "ay");
    if (r < 0) {
        print_and_warn(error.message, ERR_LINE);
        goto finish;
    }
    r = sd_bus_message_enter_container(mess, SD_BUS_TYPE_ARRAY, "y");
    if (r < 0) {
        print_and_warn(strerror(-r), ERR_LINE);
        goto finish;
    }
    while ((sd_bus_message_read(mess, "y", &bytes)) > 0) {
        sprintf(s + strlen(s), "%c", (char)bytes);
    }

finish:
    close_bus(&error, mess, iso_bus);
}

/*
 * Starts udisks2 bus monitor and returns the
 * fd associated with the bus connection.
 */
int start_monitor(void) {
    udev = udev_new();
    if (!udev) {
        WARN("could not create udev.");
        goto fail;
    }
    if (init_mbus()) {
        goto fail;
    }
    enumerate_block_devices();
    if (quit) {
        goto fail;
    }
    device_mode = DEVMON_READY;
    sd_bus_process(bus, NULL);
    INFO("started device monitor.");
    return sd_bus_get_fd(bus);

fail:
    if (!quit) {    /* else it is called by program_quit */
        free_device_monitor();
    }
    device_mode = DEVMON_OFF;
    return -1;
}

/*
 * Add matches to bus; first 3 matches are related to udisks2 signals,
 * last match is related to upower signals.
 * If last add_match fails, it won't be fatal (ie: monitor will start anyway)
 */
static int init_mbus(void) {
    int r;

    r = sd_bus_open_system(&bus);
    if (r < 0) {
        goto fail;
    }
    r = check_udisks();
    if (r < 0) {
        WARN("UDisks2 not present. Disabling udisks2 monitor.");
        goto fail;
    }
    INFO("found udisks2.");
    r = sd_bus_add_match(bus, NULL,
                        "type='signal',"
                        "sender='org.freedesktop.UDisks2',"
                        "interface='org.freedesktop.DBus.ObjectManager',"
                        "member='InterfacesAdded'",
                        add_callback, NULL);
    if (r < 0) {
        goto fail;
    }
    r = sd_bus_add_match(bus, NULL,
                    "type='signal',"
                    "sender='org.freedesktop.UDisks2',"
                    "interface='org.freedesktop.DBus.ObjectManager',"
                    "member='InterfacesRemoved'",
                    remove_callback, NULL);
    if (r < 0) {
        goto fail;
    }
    r = sd_bus_add_match(bus, NULL,
                        "type='signal',"
                        "sender='org.freedesktop.UDisks2',"
                        "interface='org.freedesktop.DBus.Properties',"
                        "member='PropertiesChanged'",
                        change_callback, NULL);
    if (r < 0) {
        goto fail;
    }
    r = sd_bus_add_match(bus, NULL,
                         "type='signal',"
                         "sender='org.freedesktop.UPower',"
                         "interface='org.freedesktop.DBus.Properties',"
                         "member='PropertiesChanged'",
                         change_power_callback, NULL);
    if (r < 0) {
        WARN("UPower PropertiesChanged signals disabled.");
    }
    return 0;

fail:
    WARN(strerror(-r));
    return r;
}

/*
 * Ping udisks2 just to check if it is present
 */
static int check_udisks(void) {
    INFO("checking for udisks2");
    return sd_bus_call_method(bus,
                            "org.freedesktop.UDisks2",
                            "/org/freedesktop/UDisks2",
                            "org.freedesktop.DBus.Peer",
                            "Ping",
                            NULL,
                            NULL,
                            NULL);
}

/*
 * Once received InterfaceAdded signal,
 * call add_device on the newly attached device.
 */
static int add_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int r;
    const char *path;
    const char *obj = "/org/freedesktop/UDisks2/block_devices/";
    char devname[PATH_MAX + 1];
    char name[NAME_MAX + 1];
    struct udev_device *dev = NULL;

    r = sd_bus_message_read(m, "o", &path);
    if (r < 0) {
        WARN(strerror(-r));
    } else {
        if (!strncmp(obj, path, strlen(obj))) {
            INFO("InterfaceAdded signal received!");
            strcpy(name, strrchr(path, '/') + 1);
            dev = udev_device_new_from_subsystem_sysname(udev, "block", name);
            if (dev) {
                sprintf(devname, "/dev/%s", name);
                r = add_device(dev, devname);
                udev_device_unref(dev);
                print_info("New device connected.", INFO_LINE);
                if (!quit && device_mode > DEVMON_READY && r != -1) {
                    update_special_mode(number_of_devices, device_mode - 1, my_devices);
                }
            }
        } else {
            INFO("signal discarded.");
        }
    }
    return 0;
}

/*
 * Once received the InterfaceRemoved signal,
 * call remove_device on the dev.
 */
static int remove_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int r;
    const char *path;
    const char *obj = "/org/freedesktop/UDisks2/block_devices/";
    char devname[PATH_MAX + 1];
    char name[NAME_MAX + 1];

    r = sd_bus_message_read(m, "o", &path);
    if (r < 0) {
        WARN(strerror(-r));
    } else {
        if (!strncmp(obj, path, strlen(obj))) {
            INFO("InterfaceRemoved signal received!");
            strcpy(name, strrchr(path, '/') + 1);
            sprintf(devname, "/dev/%s", name);
            r = remove_device(devname);
            if (!quit && device_mode > DEVMON_READY && r != -1) {
                update_special_mode(number_of_devices, device_mode - 1, my_devices);
            }
        } else {
            INFO("signal discarded.");
        }
    }
    return 0;
}

/*
 * If message received == org.freedesktop.UDisks2.Filesystem, then
 * only possible signal is "mounted status has changed".
 * So, change mounted status of device (device name =  sd_bus_message_get_path(m))
 */
static int change_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int r;
    const char *path;
    const char *obj = "org.freedesktop.UDisks2.Filesystem";
    char devname[PATH_MAX + 1];

    r = sd_bus_message_read(m, "s", &path);
    if (r < 0) {
        WARN(strerror(-r));
    } else {
        if (!strncmp(obj, path, strlen(obj))) {
            INFO("PropertiesChanged UDisks2 signal received!");
            const char *name = sd_bus_message_get_path(m);
            sprintf(devname, "/dev/%s", strrchr(name, '/') + 1);
            int present = is_present(devname);
            if (present != -1) {
                change_mounted_status(present, devname);
                if (!quit && device_mode > DEVMON_READY) {
                    update_special_mode(number_of_devices, device_mode - 1, NULL);
                }
            }
        } else {
            INFO("signal discarded.");
        }
    }
    return 0;
}

/*
 * Monitor UPower too, for AC (dis)connected events.
 * It won't monitor battery level changes (THIS IS NOT A BATTERY MONITOR!).
 * After receiving a signal, just calls timer_func() to update time/battery.
 */
static int change_power_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    const char *obj = "/org/freedesktop/UPower";

    const char *path = sd_bus_message_get_path(m);
    if (!strcmp(path, obj)) {
        INFO("PropertiesChanged UPower signal received!");
        timer_func();
    }
    return 0;
}

void devices_bus_process(void) {
    sd_bus_process(bus, NULL);
}

/*
 * Updates current tab str_ptr and number of files,
 * set device_mode and special_mode bits,
 * change tab title, and calls reset_win()
 */
void show_devices_tab(void) {
    if (number_of_devices) {
        device_mode = 1 + active;
        show_special_tab(number_of_devices, my_devices, device_mode_str);
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
 * Check through udisks2 for at least 1 mountpoint.
 * If there is no mountpoint, device is not mounted.
 * If sd_bus_get_property failes with "-EINVAL" error code,
 * it means there's no a mountable fs on the device and i won't list it.
 */
static int is_mounted(const char *dev_path) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *mess = NULL;
    int r, ret = 0;
    char obj_path[PATH_MAX + 1] = "/org/freedesktop/UDisks2/block_devices/";
    const uint8_t bytes;

    strcat(obj_path, strrchr(dev_path, '/') + 1);
    INFO("getting MountPoints property on bus.");
    r = sd_bus_get_property(bus,
                            "org.freedesktop.UDisks2",
                            obj_path,
                            "org.freedesktop.UDisks2.Filesystem",
                            "MountPoints",
                            &error,
                            &mess,
                            "aay");
    if (r < 0) {
        ret = -1;
        if (r == -EINVAL) {
            INFO("not mountable fs. Discarded.");
        } else {
            WARN(error.message);
        }
        goto finish;
    }
    r = sd_bus_message_enter_container(mess, SD_BUS_TYPE_ARRAY, "ay");
    if (r < 0) {
        goto fail;
    }
    r = sd_bus_message_enter_container(mess, SD_BUS_TYPE_ARRAY, "y");
    if (r < 0) {
        goto fail;
    }
    if (sd_bus_message_read(mess, "y", &bytes) > 0) {
        ret = 1;
    }
    goto finish;

fail:
    print_and_warn(strerror(-r), ERR_LINE);

finish:
    close_bus(&error, mess, NULL);
    return ret;
}

/*
 * Getting mountpoint for a device through mtab.
 */
static void get_mount_point(const char *dev_path, char *path) {
    struct mntent *part;
    FILE *mtab;

    if (!(mtab = setmntent("/proc/mounts", "r"))) {
        WARN("could not find /proc/mounts");
        print_info("could not find /proc/mounts.", ERR_LINE);
        return;
    }
    while ((part = getmntent(mtab))) {
        if ((part->mnt_fsname) && (!strcmp(part->mnt_fsname, dev_path))) {
            strcpy(path, part->mnt_dir);
            break;
        }
    }
    endmntent(mtab);
}

/*
 * Check what action should be performed (mount or unmount),
 * if unmount action, checks if tabs are inside the mounted device;
 * if that's the case, change cwd to path just before the mountpoint,
 * then proceed with the action.
 * If not active tab was inside mounted folder, after the unmount action,
 * it moves the tab to the path just before the mountpoint.
 */
void manage_mount_device(void) {
    int mount, r = -1;
    int pos = ps[active].curr_pos;
    int len = strlen(my_devices[pos]);
    char *ptr = strchr(my_devices[pos], ',');
    char name[PATH_MAX + 1], mounted_path[PATH_MAX + 1] = {0};

    mount = my_devices[pos][len - 1] - '0';
    strcpy(name, my_devices[pos]);
    name[len - strlen(ptr)] = '\0';
    get_mount_point(name, mounted_path);
    if (!strcmp(mounted_path, "/")) {
        print_info("Cannot unmount root.", ERR_LINE);
    } else {
        if (mount) {
            r = check_cwd(mounted_path);
            int ret = mount_fs(name, "Unmount", mount);
            if (ret != -1) {
                if (r) {
                    change_dir(mounted_path, !active);
                }
            }
        } else {
            mount_fs(name, "Mount", mount);
        }
    }
}

/*
 * For each tab, checks if it is inside mounted_path.
 * If the active tab is inside, calculate the path just before the mountpoint,
 * and chdir there. Else, returns 1.
 */
static int check_cwd(char *mounted_path) {
    int len, ret = 0;

    for (int i = 0; i < cont; i++) {
        if (!strncmp(ps[i].my_cwd, mounted_path, strlen(mounted_path))) {
            if (i == active) {
                len = strlen(strrchr(mounted_path, '/'));
                len = strlen(mounted_path) - len;
                mounted_path[len] = '\0';
                chdir(mounted_path);
                strcpy(ps[active].my_cwd, mounted_path);
            } else {
                ret = 1;
            }
        }
    }
    return ret;
}

/*
 * Pressing enter on a device will move to its mountpoint.
 * If device is not mounted, it will mount it too.
 */
void manage_enter_device(void) {
    int mount, ret = 1;
    int pos = ps[active].curr_pos;
    int len = strlen(my_devices[pos]);
    char *ptr = strchr(my_devices[pos], ',');
    char dev_path[PATH_MAX + 1], name[PATH_MAX + 1] = {0};

    mount = my_devices[pos][len - 1] - '0';
    strcpy(dev_path, my_devices[pos]);
    dev_path[len - strlen(ptr)] = '\0';
    if (!mount) {
        ret = mount_fs(dev_path, "Mount", mount);
    }
    if (ret == 1) {
        get_mount_point(dev_path, name);
        strcpy(ps[active].my_cwd, name);
        leave_device_mode();
    }
}

static void change_mounted_status(int pos, const char *name) {
    int len = strlen(my_devices[pos]);
    int mount = my_devices[pos][len - 1] - '0';
    sprintf(my_devices[pos] + len - 1, "%d", !mount);
    if (!strlen(mount_str)) {
        if (mount == 1) {
            sprintf(mount_str, "External tool has mounted %s.", name);
        } else {
            sprintf(mount_str, "External tool has unmounted %s.", name);
        }
    }
    print_info(mount_str, INFO_LINE);
    memset(mount_str, 0, strlen(mount_str));
}

void leave_device_mode(void) {
    device_mode = DEVMON_READY;
    change_dir(ps[active].my_cwd, active);
}

void free_device_monitor(void) {
    INFO("freeing device monitor resources...");
    if (bus) {
        sd_bus_flush_close_unref(bus);
    }
    if (my_devices) {
        free(my_devices);
    }
    if (udev) {
        udev_unref(udev);
    }
    INFO("freed.");
}

/*
 * Check if dev must be ignored, then
 * check if it is a mountable fs (eg: a dvd reader without a dvd inserted, is NOT a moutable fs),
 * and its mount status. Then add the device to my_devices.
 * If the dev is a loop_dev or if config.automount is == 1 and the dev is not mounted,
 * it will mount the device.
 */
static int add_device(struct udev_device *dev, const char *name) {
    int mount;
    uint64_t size;
    char s[20];
    const char *ignore = udev_device_get_property_value(dev, "UDISKS_IGNORE");

    if (ignore && !strcmp(ignore, "1")) {
        mount = -1;
    } else {
        mount = is_mounted(name);
    }
    if (mount != -1) {
        my_devices = safe_realloc((PATH_MAX + 1) * (number_of_devices + 1));
        if (!quit) {
            /* calculates current device size */
            size = strtol(udev_device_get_sysattr_value(dev, "size"), NULL, 10);
            size = (size / (uint64_t) 2) * 1024;
            change_unit((float)size, s);
            if (udev_device_get_property_value(dev, "ID_MODEL")) {
                sprintf(my_devices[number_of_devices], "%s, %s, Size: %s, Mounted: %d",
                        name, udev_device_get_property_value(dev, "ID_MODEL"), s, mount);
            } else {
                sprintf(my_devices[number_of_devices], "%s, Size: %s, Mounted: %d",
                        name, s, mount);
            }
            number_of_devices++;
            INFO("added device.");
            int is_loop_dev = !strncmp(name, "/dev/loop", strlen("/dev/loop"));
            if ((device_mode != DEVMON_STARTING) && ((is_loop_dev) || (!mount && config.automount))) {
                    mount_fs(name, "Mount", mount);
            }
        }
    }
    return mount;
}

static int remove_device(const char *name) {
    int i = is_present(name);

    if (i != -1) {
        memmove(&my_devices[i], &my_devices[i + 1], (number_of_devices - 1 - i) * sizeof(my_devices[0]));
        my_devices = safe_realloc(PATH_MAX * (number_of_devices - 1));
        if (!quit) {
            number_of_devices--;
            print_info("Device removed.", INFO_LINE);
            INFO("removed device.");
        }
    }
    return i;
}

static int is_present(const char *name) {
    for (int i = 0; i < number_of_devices; i++) {
        if (strncmp(my_devices[i], name, strlen(name)) == 0) {
            return i;
        }
    }
    return -1;
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

#endif
#endif
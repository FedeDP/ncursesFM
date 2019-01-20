#include "../inc/devices.h"

static int mount_fs(const char *str, int mount);
static void set_autoclear(const char *loopname);
static int is_iso_mounted(const char *filename, char *loop_dev);
static void iso_backing_file(char *s, const char *name);
static int init_mbus(void);
static int check_udisks(void);
static int add_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int remove_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int change_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int change_power_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static void enumerate_block_devices(void);
static int get_mount_point(const char *dev_path, char *path);
static void change_mounted_status(int pos, const char *name);
static void fix_tab_cwd(void);
static int add_device(struct udev_device *dev, const char *name);
static int remove_device(const char *name);

static char mount_str[PATH_MAX + 1];
static struct udev *udev;
static int number_of_devices;
static char (*devices)[PATH_MAX + 1];
static sd_bus *bus;

/*
 * Open the bus, and call "method" string on UDisks2.Filesystem.
 */
static int mount_fs(const char *str, int mount) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *mess = NULL;
    sd_bus *mount_bus = NULL;
    const char *path;
    char obj_path[PATH_MAX + 1] = "/org/freedesktop/UDisks2/block_devices/";
    char tmp[30], method[10];
    int r, ret = -1;
    char mounted_path[PATH_MAX + 1] = {0};
    int len, len2;

    r = sd_bus_open_system(&mount_bus);
    if (r < 0) {
        print_and_warn(strerror(-r), ERR_LINE);
        goto finish;
    }
    if (mount) {
        if (get_mount_point(str, mounted_path) == -1) {
            ERROR("Could not get mount point.");
        }
        // calculate root dir of mounted path
        strncpy(mounted_path, dirname(mounted_path), PATH_MAX);
        len = strlen(mounted_path);
        len2 = strlen(ps[active].my_cwd);
        // if active win is inside mounted path
        if (!strncmp(ps[active].my_cwd, mounted_path, len) && len2 > len) {
            // move away process' cwd from mounted path as it would raise an error while unmounting
            chdir(mounted_path);
        }
        strcpy(method, "Unmount");
    } else {
        strcpy(method, "Mount");
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
        /* if it was not succesful, restore old_cwd */
        chdir(ps[active].my_cwd);
        print_and_warn(error.message, ERR_LINE);
        goto finish;
    }
    ret = 1;
    if (!mount) {
        sd_bus_message_read(mess, "s", &path);
        snprintf(mount_str, PATH_MAX, _(dev_mounted), str, path);
        INFO("Mounted.");
        /* if it's a loop dev, set autoclear after mounting it */
        if (!strncmp(str, "/dev/loop", strlen("/dev/loop"))) {
            set_autoclear(str);
        }
    } else {
        /*
         * save back in ps[active].my_cwd, process' cwd
         * to be sure we carry the right path
         */
        getcwd(ps[active].my_cwd, PATH_MAX);
        snprintf(mount_str, PATH_MAX, _(dev_unmounted), str);
        INFO("Unmounted.");
    }

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
    char loop_dev[PATH_MAX + 1];
    
    int mount = is_iso_mounted(str, loop_dev);
    /* it was already present */
    if (mount != -1) {
        /* 
         * if it was mounted, then unmount it.
         * if it is was unmounted, just mount it (ie: someone else
         * created this loop dev probably without autoclear true)
         */
        mount_fs(loop_dev, mount);
        return;
    }
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
    if (r < 0) {
        print_and_warn(error.message, ERR_LINE);
        goto finish;
    }
    sd_bus_message_read(mess, "o", &obj_path);

finish:
    close(fd);
    close_bus(&error, mess, iso_bus);
}

static void set_autoclear(const char *loopname) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *mess = NULL;
    sd_bus *iso_bus = NULL;
    int r;
    
    r = sd_bus_open_system(&iso_bus);
    if (r < 0) {
        print_and_warn(strerror(-r), ERR_LINE);
    } else {
        char obj_path[PATH_MAX + 1] = "/org/freedesktop/UDisks2/block_devices/";
        
        strcat(obj_path, strrchr(loopname, '/') + 1);
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
    }
    close_bus(&error, mess, iso_bus);
}

/*
 * Check if iso is already mounted.
 * For each /dev/loop device present, calls iso_backing_file on bus.
 * As soon as it finds a matching backing_file with current file's filename,
 * it breaks the cycle and returns its mounted status;
 */
static int is_iso_mounted(const char *filename, char loop_dev[PATH_MAX + 1]) {
    struct udev *tmp_udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devlist, *dev_list_entry;
    char s[PATH_MAX + 1] = {0};
    char resolved_path[PATH_MAX + 1];
    int mount = -1;
    
    realpath(filename, resolved_path);
    tmp_udev = udev_new();
    enumerate = udev_enumerate_new(tmp_udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_add_match_property(enumerate, "ID_FS_TYPE", "udf");
    udev_enumerate_scan_devices(enumerate);
    devlist = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devlist) {
        const char *path = udev_list_entry_get_name(dev_list_entry);
        struct udev_device *dev = udev_device_new_from_syspath(tmp_udev, path);
        strncpy(loop_dev, udev_device_get_devnode(dev), PATH_MAX);
        iso_backing_file(s, loop_dev);
        udev_device_unref(dev);
        if (!strcmp(s, resolved_path)) {
            mount = get_mount_point(loop_dev, NULL);
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
    uint8_t bytes = '\0';
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
    device_init = DEVMON_READY;
    INFO("started device monitor.");
    return sd_bus_get_fd(bus);

fail:
    device_init = DEVMON_OFF;
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
    const char obj[] = "/org/freedesktop/UDisks2/block_devices/";
    char devname[PATH_MAX + 1] = {0};
    char name[NAME_MAX + 1] = {0};
    struct udev_device *dev;

    r = sd_bus_message_read(m, "o", &path);
    if (r < 0) {
        WARN(strerror(-r));
    } else {
        if (!strncmp(obj, path, strlen(obj))) {
            INFO("InterfaceAdded signal received!");
            strncpy(name, strrchr(path, '/') + 1, NAME_MAX);
            dev = udev_device_new_from_subsystem_sysname(udev, "block", name);
            if (dev) {
                snprintf(devname, PATH_MAX, "/dev/%s", name);
                r = add_device(dev, devname);
                udev_device_unref(dev);
                if (!quit && r != -1) {
                    update_special_mode(number_of_devices, devices, device_);
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
    const char obj[] = "/org/freedesktop/UDisks2/block_devices/";
    char devname[PATH_MAX + 1] = {0};
    char name[NAME_MAX + 1] = {0};

    r = sd_bus_message_read(m, "o", &path);
    if (r < 0) {
        WARN(strerror(-r));
    } else {
        if (!strncmp(obj, path, strlen(obj))) {
            INFO("InterfaceRemoved signal received!");
            strncpy(name, strrchr(path, '/') + 1, NAME_MAX);
            snprintf(devname, PATH_MAX, "/dev/%s", name);
            r = remove_device(devname);
            if (!quit && r != -1) {
                update_special_mode(number_of_devices, devices, device_);
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
 * So, change mounted status of device (device name = sd_bus_message_get_path(m))
 */
static int change_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int r;
    const char *path;
    const char obj[] = "org.freedesktop.UDisks2.Filesystem";
    char devname[PATH_MAX + 1] = {0};

    r = sd_bus_message_read(m, "s", &path);
    if (r < 0) {
        WARN(strerror(-r));
    } else {
        if (!strncmp(obj, path, strlen(obj))) {
            INFO("PropertiesChanged UDisks2 signal received!");
            const char *name = sd_bus_message_get_path(m);
            snprintf(devname, PATH_MAX, "/dev/%s", strrchr(name, '/') + 1);
            int present = is_present(devname, devices, number_of_devices, strlen(devname), 0);
            if (present != -1) {
                change_mounted_status(present, devname);
                update_special_mode(present, NULL, device_);
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
 * After receiving a signal, just calls timer_event() to update time/battery.
 */
static int change_power_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    const char obj[] = "/org/freedesktop/UPower";

    const char *path = sd_bus_message_get_path(m);
    if (!strcmp(path, obj)) {
        INFO("PropertiesChanged UPower signal received!");
        timer_event();
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
        show_special_tab(number_of_devices, devices, device_mode_str, device_);
    } else {
        print_info(_(no_devices), INFO_LINE);
    }
}

/*
 * Scan "block" subsystem for devices.
 * For each device check if it is a really mountable external fs,
 * add it to devices{} and increment number_of_devices.
 */
static void enumerate_block_devices(void) {
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devlist, *dev_list_entry;

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    devlist = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devlist) {
        const char *path = udev_list_entry_get_name(dev_list_entry);
        struct udev_device *dev;
        
        if ((dev = udev_device_new_from_syspath(udev, path))) {
            const char *name = udev_device_get_devnode(dev);
            add_device(dev, name);
            udev_device_unref(dev);
            if (quit) {
                break;
            }
        }
    }
    udev_enumerate_unref(enumerate);
}

/*
 * Getting mountpoint for a device through mtab.
 */
static int get_mount_point(const char *dev_path, char *path) {
    struct mntent *part;
    FILE *mtab;
    int ret = 0;

    if (!(mtab = setmntent("/proc/mounts", "r"))) {
        WARN(no_proc_mounts);
        print_info(_(no_proc_mounts), ERR_LINE);
        return -1;
    }
    while ((part = getmntent(mtab))) {
        if ((part->mnt_fsname) && (!strcmp(part->mnt_fsname, dev_path))) {
            if (path) {
                strncpy(path, part->mnt_dir, PATH_MAX);
            }
            ret = 1;
            break;
        }
    }
    endmntent(mtab);
    return ret;
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
    int mount;
    int pos = ps[active].curr_pos;
    int len = strlen(devices[pos]);
    char *ptr = strchr(devices[pos], ',');
    char name[PATH_MAX + 1];

    strncpy(name, devices[pos], PATH_MAX);
    name[len - strlen(ptr)] = '\0';
    mount = devices[pos][len - 1] - '0';
    mount_fs(name, mount);
}

/*
 * Pressing enter on a device will move to its mountpoint.
 * If device is not mounted, it will mount it too.
 */
void manage_enter_device(void) {
    int mount, ret = 1;
    int pos = ps[active].curr_pos;
    int len = strlen(devices[pos]);
    char *ptr = strchr(devices[pos], ',');
    char dev_path[PATH_MAX + 1] = {0}, name[PATH_MAX + 1] = {0};

    mount = devices[pos][len - 1] - '0';
    strncpy(dev_path, devices[pos], PATH_MAX);
    dev_path[len - strlen(ptr)] = '\0';
    if (!mount) {
        ret = mount_fs(dev_path, mount);
    }
    if (ret == 1 && get_mount_point(dev_path, name) != -1) {
        memset(ps[active].old_file, 0, strlen(ps[active].old_file));
        leave_special_mode(name, active);
    }
}

static void change_mounted_status(int pos, const char *name) {
    int len = strlen(devices[pos]);
    int mount = devices[pos][len - 1] - '0';
    sprintf(devices[pos] + len - 1, "%d", !mount);
    if (!strlen(mount_str)) {
        if (mount) {
            snprintf(mount_str, PATH_MAX, _(ext_dev_unmounted), name);
        } else {
            snprintf(mount_str, PATH_MAX, _(ext_dev_mounted), name);
        }
    }
    print_info(_(mount_str), INFO_LINE);
    memset(mount_str, 0, strlen(mount_str));
    // if mount == 1, it means we're unmounting
    // and we need to check !active tab
    // to see if it was inside mountpoint
    // and in case, move it away
    if (mount) {
        fix_tab_cwd();
    }
}

/*
 * If a tab was inside unmounted mount point,
 * move it away, and if it is in normal mode,
 * refresh its dir
 * 
 */
static void fix_tab_cwd(void) {
    for (int j = 0; j < cont; j++) {
        int i = 0;
        while (access(ps[j].my_cwd, F_OK) != 0 && strlen(ps[j].my_cwd)) {
            i++;
            strncpy(ps[j].my_cwd, dirname(ps[j].my_cwd), PATH_MAX);
        }
        if (i && ps[j].mode <= fast_browse_) {
            change_dir(ps[j].my_cwd, j);
        }
    }
}

void free_device_monitor(void) {
    INFO("freeing device monitor resources...");
    if (bus) {
        sd_bus_flush_close_unref(bus);
    }
    if (devices) {
        free(devices);
    }
    if (udev) {
        udev_unref(udev);
    }
    INFO("freed.");
}

/*
 * Check if dev must be ignored, then
 * check if it is a mountable fs (eg: a dvd reader without a dvd inserted, is NOT a moutable fs),
 * and its mount status. Then add the device to devices.
 * If the dev is not mounted and it is a loop_dev or if config.automount is == 1,
 * it will mount the device.
 */
static int add_device(struct udev_device *dev, const char *name) {
    int mount;
    const char *ignore = udev_device_get_property_value(dev, "UDISKS_IGNORE");
    const char *fs_type = udev_device_get_property_value(dev, "ID_FS_TYPE");

    if (!fs_type || (ignore && !strcmp(ignore, "1"))) {
        mount = -1;
    } else {
        mount = get_mount_point(name, NULL);
    }
    if (mount != -1) {
        devices = safe_realloc(number_of_devices + 1, devices);
        if (!quit) {
            if (udev_device_get_property_value(dev, "ID_MODEL")) {
                snprintf(devices[number_of_devices], PATH_MAX, "%s, %s, Mounted: %d",
                        name, udev_device_get_property_value(dev, "ID_MODEL"), mount);
            } else {
                snprintf(devices[number_of_devices], PATH_MAX, "%s, Mounted: %d",
                        name, mount);
            }
            number_of_devices++;
            INFO(device_added);
            print_info(_(device_added), INFO_LINE);
            int is_loop_dev = !strncmp(name, "/dev/loop", strlen("/dev/loop"));
            if (device_init != DEVMON_STARTING && (!mount && (config.automount || is_loop_dev))) {
                mount_fs(name, mount);
            }
        }
    }
    return mount;
}

static int remove_device(const char *name) {
    int i = is_present(name, devices, number_of_devices, strlen(name), 0);

    if (i != -1) {
        devices = remove_from_list(&number_of_devices, devices, i);
        if (!quit) {
            print_info(_(device_removed), INFO_LINE);
            INFO(device_removed);
        }
    }
    return i;
}

void show_devices_stat(int i, int win, char *str) {
    const int flags[] = {  ST_MANDLOCK, ST_NOATIME, ST_NODEV, 
                            ST_NODIRATIME, ST_NOEXEC, ST_NOSUID, 
                            ST_RDONLY, ST_RELATIME, ST_SYNCHRONOUS};
    struct statvfs stat;
    char dev_path[PATH_MAX + 1] = {0}, path[PATH_MAX + 1] = {0};
    char s[20] = {0};
    uint64_t total;
    
    int len = strlen(devices[i]);
    int mount = devices[i][len - 1] - '0';
    strncpy(dev_path, devices[i], PATH_MAX);
    char *ptr = strchr(devices[i], ',');
    dev_path[len - strlen(ptr)] = '\0';
    // if device is mounted
    if (mount && get_mount_point(dev_path, path) != -1) {
        const char *fl[] = {"mand", "noatime", "nodev", "nodiratime",  
            "noexec", "nosuid", "ro", "relatime", "sync"};

        statvfs(path, &stat);
        uint64_t free = stat.f_bavail * stat.f_bsize;
        change_unit((float) free, s);
        sprintf(str, "Free: %s/", s);
        memset(s, 0, strlen(s));
        total = stat.f_blocks * stat.f_frsize;
        change_unit((float) total, s);
        strcat(str, s);
        sprintf(str + strlen(str), ", mount opts:");
        for (int j = 0; j < 9; j++) {
            if (stat.f_flag & flags[j]) {
                sprintf(str + strlen(str), " %s", fl[j]);
            }
        }
        if (!(stat.f_flag & ST_RDONLY)) {
            sprintf(str + strlen(str), " rw");
        }
        if (!(stat.f_flag & ST_SYNCHRONOUS)) {
            sprintf(str + strlen(str), " async");
        }
    } else {
        // use udev system to get device size
        struct udev_device *dev;
        char *name = strrchr(dev_path, '/') + 1;
        dev = udev_device_new_from_subsystem_sysname(udev, "block", name);
        if (dev && udev_device_get_sysattr_value(dev, "size")) {
            total = strtol(udev_device_get_sysattr_value(dev, "size"), NULL, 10);
            total = (total / (uint64_t) 2) * (uint64_t)1024;
            change_unit((float)total, s);
            sprintf(str, "Total size: %s", s);
            udev_device_unref(dev);
        }
    }
}

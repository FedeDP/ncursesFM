#ifdef SYSTEMD_PRESENT

#include "../inc/mount.h"

static void close_bus(sd_bus_error *error, sd_bus_message *mess, sd_bus *bus);
#ifdef LIBUDEV_PRESENT
static void enumerate_usb_mass_storage(void);
static int is_mounted(const char *dev_path);
#endif

void mount_fs(const char *str, const char *method, int mount) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *mess = NULL;
    sd_bus *mount_bus = NULL;
    const char *path;
    char obj_path[PATH_MAX] = "/org/freedesktop/UDisks2/block_devices/";
    char success[PATH_MAX] = "Mounted in ";
    int r;
    
    r = sd_bus_open_system(&mount_bus);
    if (r < 0) {
        print_info(bus_error, ERR_LINE);
        return;
    }
    strcat(obj_path, strrchr(str, '/') + 1);
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
    } else {
        if (!mount) {
            sd_bus_message_read(mess, "s", &path);
            strcat(success, path);
            print_info(success, INFO_LINE);
        } else {
            print_info("Unmounted.", INFO_LINE);
        }
    }
    close_bus(&error, mess, mount_bus);
}

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
        return;
    }
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
    } else {
        sd_bus_message_read(mess, "o", &obj_path);
        mount_fs(obj_path, "Mount", 0);
        sd_bus_flush(iso_bus);
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
        }
    }
    close_bus(&error, mess, iso_bus);
}

static void close_bus(sd_bus_error *error, sd_bus_message *mess, sd_bus *bus) {
    sd_bus_message_unref(mess);
    sd_bus_error_free(error);
    sd_bus_flush_close_unref(bus);
}

#ifdef LIBUDEV_PRESENT
void devices_tab(void) {
    enumerate_usb_mass_storage();
    if (device_mode && !quit) {
        str_ptr[active] = usb_devices;
        ps[active].number_of_files = device_mode;
        device_mode = 1 + active;
        ps[active].needs_refresh = DONT_REFRESH;
        reset_win(active);
        sprintf(ps[active].title, device_mode_str);
        list_everything(active, 0, 0);
    } else {
        print_info("No mountable devices found.", INFO_LINE);
    }
}

/*
 * Scan "block" subsystem for devices.
 * For each device check if it is a really mountable external fs,
 * add it to usb_devices{} and increment device_mode.
 */
static void enumerate_usb_mass_storage(void) {
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev, *next_dev, *parent;

    udev = udev_new();
    if (!udev) {
        print_info("Can't create udev", INFO_LINE);
        return;
    }
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);
        const char *name = udev_device_get_devnode(dev);
        // just list really mountable fs (eg: if there is /dev/sdd1, do not list /dev/sdd)
        if (udev_device_get_sysnum(dev) == 0) {
            next_dev = udev_device_new_from_syspath(udev, udev_list_entry_get_name(udev_list_entry_get_next(dev_list_entry)));
            if (next_dev && (strncmp(name, udev_device_get_devnode(next_dev), strlen(name)) == 0)) {
                udev_device_unref(dev);
                udev_device_unref(next_dev);
                continue;
            }
            udev_device_unref(next_dev);
        }
        parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (parent) {
            if (!(usb_devices = realloc(usb_devices, sizeof(*(usb_devices)) * (device_mode + 1)))) {
                quit = MEM_ERR_QUIT;
                udev_device_unref(dev);
                break;
            }
            sprintf(usb_devices[device_mode], "%s, VID/PID: %s:%s, %s %s, Mounted: %d",
                    name, udev_device_get_sysattr_value(parent, "idVendor"),
                    udev_device_get_sysattr_value(parent, "idProduct"),
                    udev_device_get_sysattr_value(parent,"manufacturer"),
                    udev_device_get_sysattr_value(parent,"product"),
                    is_mounted(name));
            device_mode++;
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
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
            }
        }
        endmntent ( mtab);
    }
    return is_mounted;
}

void manage_enter_device(void) {
    int mount;
    int pos = ps[active].curr_pos;
    int len = strlen(usb_devices[pos]);
    char *ptr = strchr(usb_devices[pos], ',');

    mount = usb_devices[pos][len - 1] - '0';
    usb_devices[pos][len - strlen(ptr)] = '\0';
    if (mount) {
        mount_fs(usb_devices[pos], "Unmount", mount);
    } else {
        mount_fs(usb_devices[pos], "Mount", mount);
    }
    leave_device_mode();
}

void leave_device_mode(void) {
    free(usb_devices);
    usb_devices = NULL;
    device_mode = 0;
    change_dir(ps[active].my_cwd);
}
#endif

#endif
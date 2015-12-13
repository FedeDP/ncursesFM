#include "fm_functions.h"

#ifdef SYSTEMD_PRESENT

#include <systemd/sd-bus.h>

void mount_fs(const char *str, const char *method, int mount);
void isomount(const char *str);
#endif

#ifdef LIBUDEV_PRESENT

#include <mntent.h>
#include <libudev.h>

void start_udev(void);
void show_devices_tab(void);
void manage_enter_device(void);
void leave_device_mode(void);
#endif

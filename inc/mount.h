#ifdef SYSTEMD_PRESENT

#include "fm_functions.h"
#include <systemd/sd-bus.h>

void isomount(const char *str);

#ifdef LIBUDEV_PRESENT

#include <libudev.h>
#include <mntent.h>

int start_monitor(void);
void devices_bus_process(void);
void show_devices_tab(void);
void manage_mount_device(void);
void manage_enter_device(void);
void leave_device_mode(void);
void free_device_monitor(void);
#endif
#endif

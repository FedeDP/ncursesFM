#include "fm.h"

#include <systemd/sd-bus.h>
#include <libudev.h>
#include <sys/statvfs.h>
#include <mntent.h>

void isomount(const char *str);
int start_monitor(void);
void devices_bus_process(void);
void show_devices_tab(void);
void manage_mount_device(void);
void manage_enter_device(void);
void free_device_monitor(void);
void show_devices_stat(int i, int win, char *str);

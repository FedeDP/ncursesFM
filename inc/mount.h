#include "fm_functions.h"

#ifdef SYSTEMD_PRESENT

#include <systemd/sd-bus.h>
#include <libudev.h>
#include <libmount/libmount.h>

void isomount(const char *str);
int start_monitor(void);
void devices_bus_process(void);
void show_devices_tab(void);
void manage_mount_device(void);
void manage_enter_device(void);
void leave_device_mode(void);
void free_device_monitor(void);

#endif

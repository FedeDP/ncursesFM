#include "fm_functions.h"

#ifdef SYSTEMD_PRESENT

#include <systemd/sd-bus.h>

void isomount(const char *str);

#ifdef LIBUDEV_PRESENT

#include <libudev.h>

void start_monitor(void);
void show_devices_tab(void);
void manage_enter_device(void);
void leave_device_mode(void);
#endif
#endif

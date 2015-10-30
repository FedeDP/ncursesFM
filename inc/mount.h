#include <systemd/sd-bus.h>
#include <mntent.h>
#include "fm_functions.h"

void mount_fs(const char *str, const char *method, int mount);
void isomount(const char *str);

#ifdef LIBUDEV_PRESENT

#include <libudev.h>

void devices_tab(void);
void manage_enter_device(void);
void leave_device_mode(void);
#endif

#include <systemd/sd-bus.h>
#include <fcntl.h>
#include "ui_functions.h"

int inhibit_suspend(const char *str);
void close_bus(sd_bus_error *error, sd_bus_message *mess, sd_bus *bus);
void stop_inhibition(int fd);
#pragma once

#include <libnotify/notify.h>

void init_notify(void);
void send_notification(const char *mesg);
void destroy_notify(void);

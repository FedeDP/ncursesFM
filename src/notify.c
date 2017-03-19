#ifdef LIBNOTIFY_PRESENT

#include "../inc/notify.h"

static NotifyNotification *n = NULL;
static const char title[] = "NcursesFM";

void init_notify(void) {
    notify_init(title);
}

void send_notification(const char *mesg) {
    if (!config.silent && has_desktop) {
        n = notify_notification_new(title, mesg, 0);
        notify_notification_set_timeout(n, NOTIFY_EXPIRES_DEFAULT);
        notify_notification_show(n, 0);
    }
}

void destroy_notify(void) {
    if (n) {
        notify_notification_close(n, NULL);
    }
    notify_uninit();
}

#endif

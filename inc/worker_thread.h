#pragma once

#ifdef SYSTEMD_PRESENT
#include "inhibit.h"
#else
#include "ui.h"
#endif

#ifdef LIBNOTIFY_PRESENT
#include "notify.h"
#endif

struct thread_mesg {
    const char *str;
    int line;
};

void init_job_queue(void);
void destroy_job_queue(void);
void init_thread(int type, int (* const f)(void));

#pragma once

#ifdef SYSTEMD_PRESENT
#include "inhibit.h"
#else
#include "ui_functions.h"
#endif

struct thread_mesg {
    const char *str;
    int line;
};

void init_thread(int type, int (* const f)(void));

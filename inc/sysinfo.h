#include "ui.h"
#include <sys/timerfd.h>
#include <libudev.h>

int start_timer(void);
void timer_func(void);
void free_timer(void);

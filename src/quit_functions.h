#include <sys/stat.h>
#include "helper_functions.h"

void free_everything(void);
void quit_thread_func(pthread_t tmp);
void free_thread_list(thread_l *h);
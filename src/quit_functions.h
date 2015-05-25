#include <pthread.h>
#include <signal.h>
#include "helper_functions.h"

void free_copied_list(file_list *h);
void free_everything(void);
void quit_thread_func(void);
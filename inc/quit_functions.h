#ifndef QUIT_H
#define QUIT_H

#include "ui_functions.h"
#include <signal.h>

int program_quit(void);
void free_copied_list(file_list *h);

#endif
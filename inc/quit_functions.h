#pragma once

#include "ui_functions.h"
#include <signal.h>

int program_quit(int sig_received);
void free_copied_list(file_list *h);
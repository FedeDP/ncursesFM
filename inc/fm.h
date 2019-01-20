#pragma once

#include "install_package.h"
#include "search.h"
#include "archiver.h"
#include "worker_thread.h"

#include <wchar.h>
#include <linux/version.h>
#include <sys/wait.h>
#include <sys/time.h>

#ifdef LIBCUPS_PRESENT
#include "print.h"
#endif

int change_dir(const char *str, int win);
void change_tab(void);
void switch_hidden(void);
void manage_file(const char *str);
void fast_file_operations(const int index);
int remove_file(void);
void manage_space_press(const char *str);
void manage_all_space_press(void);
void remove_selected(void);
void remove_all_selected(void);
void show_selected(void);
void free_selected(void);
int paste_file(void);
int move_file(void);
void fast_browse(wint_t c);


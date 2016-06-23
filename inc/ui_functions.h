#pragma once

#include "timer.h"
#include "mount.h"
#include "string_constants.h"
#include "quit_functions.h"
#include "utils.h"

#include <locale.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/sysinfo.h>
#include <wctype.h>
#include <langinfo.h>

#define INFO_HEIGHT 4
#define SEL_COL 3
#define STAT_LENGTH 20
#define PERM_LENGTH 10

// colors for active win / fast browse mode
#define ACTIVE_COL 3
#define FAST_BROWSE_COL 5

/*
 * inotify macros
 */
#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))

void screen_init(void);
void screen_end(void);
void reset_win(int win);
void new_tab(int win);
void resize_tab(int win, int resizing);
void delete_tab(int win);
void scroll_down(int win, int lines);
void scroll_up(int win, int lines);
void trigger_show_helper_message(void);
void trigger_stats(void);
wint_t main_poll(WINDOW *win);
void timer_event(void);
void tab_refresh(int win);
void update_special_mode(int num, char (*str)[PATH_MAX + 1], int mode);
void show_special_tab(int num, char (*str)[PATH_MAX + 1], const char *title, int mode);
void leave_special_mode(const char *str, int win);
void print_info(const char *str, int i);
void print_and_warn(const char *err, int line);
void ask_user(const char *str, char *input, int d);
void resize_win(void);
void change_sort(void);
void highlight_selected(const char *str, const char c, int win);
void erase_selected_highlight(void);
void update_colors(void);
void update_time(int where);
void update_sysinfo(int where);
void update_batt(int online, int perc[], int num_of_batt, char name[][10], int where);
void trigger_fullname_win(void);

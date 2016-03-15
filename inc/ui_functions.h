#pragma once

#include "log.h"
#include "timer.h"
#include "mount.h"
#include "string_constants.h"
#include "quit_functions.h"
#include <locale.h>
#include <stdlib.h>
#include <ctype.h>
#include <archive.h>
#include <archive_entry.h>
#include <sys/sysinfo.h>

#define INFO_HEIGHT 4
#define SEL_COL 3
#define STAT_LENGTH 20
#define PERM_LENGTH 10

#define STATS_OFF 0
#define STATS_ON 1
#define STATS_IDLE 2

#define ACTIVE_COL 2
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
void resize_tab(int win);
void delete_tab(int win);
void scroll_down(int win);
void scroll_up(int win);
void trigger_show_helper_message(void);
void trigger_stats(void);
void change_unit(float size, char *str);
int main_poll(WINDOW *win);
void tab_refresh(int win);
void update_special_mode(int num, int win, char (*str)[PATH_MAX + 1]);
void show_special_tab(int num, char (*str)[PATH_MAX + 1], const char *title, int mode);
void leave_special_mode(const char *str);
void print_info(const char *str, int i);
void print_and_warn(const char *err, int line);
void ask_user(const char *str, char *input, int d, char c);
void resize_win(void);
void change_sort(void);
void highlight_selected(int line, const char c);
void erase_selected_highlight(void);
void update_arrows(void);
void switch_fast_browse_mode(void);
void update_time(void);
void update_batt(int online, int perc[], int num_of_batt, char name[][10]);
void trigger_fullname_win(void);

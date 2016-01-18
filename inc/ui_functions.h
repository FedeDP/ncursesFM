#pragma once

#include "log.h"
#include "time.h"
#include "mount.h"
#include "string_constants.h"
#include "quit_functions.h"
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <ctype.h>
#include <archive.h>
#include <archive_entry.h>

#define INFO_HEIGHT 4
#define SEL_COL 3
#define STAT_LENGTH 20
#define PERM_LENGTH 10

#define STATS_OFF 0
#define STATS_ON 1
#define STATS_IDLE 2

#define ACTIVE_COL 2
#define FAST_BROWSE_COL 5

void screen_init(void);
void screen_end(void);
void reset_win(int win);
void new_tab(int win);
void change_first_tab_size(void);
void delete_tab(int win);
void scroll_down(void);
void scroll_up(void);
void trigger_show_helper_message(void);
void trigger_stats(void);
void change_unit(float size, char *str);
int win_getch(WINDOW *win);
void tab_refresh(int win);
void update_special_mode(int num, int win, char (*str)[PATH_MAX + 1]);
void show_special_tab(int num, char (*str)[PATH_MAX + 1], const char *title);
void print_info(const char *str, int i);
void print_and_warn(const char *err, int line);
void ask_user(const char *str, char *input, int d, char c);
void resize_win(void);
void change_sort(void);
void highlight_selected(int line, const char c);
void erase_selected_highlight(void);
void change_tab(void);
void switch_fast_browse_mode(void);
void update_time(const char *ac_path, struct supply *batt, int num_of_batt);
int query_file(const char *path, const char *query, int *result);
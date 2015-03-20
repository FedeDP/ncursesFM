#include <stdbool.h>
#include "declarations.h"
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

void screen_init(void);
void screen_end(void);
void list_everything(int win, int old_dim, int end, int erase, int reset);
int is_hidden(const struct dirent *current_file);
void my_sort(int win);
void new_tab(void);
void delete_tab(void);
void scroll_down(char *str);
void scroll_up(char *str);
void scroll_helper_func(int x, int direction);
void sync_changes(void);
void colored_folders(int i, int win);
void print_info(char *str, int i);
void clear_info(int i);
void trigger_show_helper_message(void);
void helper_print(void);
void show_stat(int init, int end, int win);
void set_nodelay(bool x);
#include "quit_functions.h"

#define INFO_HEIGHT 2
#define HELPER_HEIGHT 15
#define STAT_COL 30
#define MAX_FILENAME_LENGTH 25

void screen_init(void);
void screen_end(void);
void generate_list(int win);
void list_everything(int win, int old_dim, int end, char **files);
static int is_hidden(const struct dirent *current_file);
static void my_sort(int win);
void new_tab(void);
void delete_tab(void);
void scroll_down(char **str);
void scroll_up(char **str);
static void scroll_helper_func(int x, int direction);
void sync_changes(void);
static void colored_folders(int win, char *name);
void trigger_show_helper_message(void);
static void helper_print(void);
void show_stat(int init, int end, int win);
void erase_stat(void);
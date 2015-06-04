#include "quit_functions.h"

#define INFO_HEIGHT 2
#define HELPER_HEIGHT 14
#define STAT_COL 30
#define MAX_FILENAME_LENGTH 25

void screen_init(void);
void screen_end(void);
void generate_list(int win);
void list_everything(int win, int old_dim, int end, char **files);
void new_tab(void);
void delete_tab(void);
void scroll_down(char **str);
void scroll_up(char **str);
void sync_changes(void);
void trigger_show_helper_message(void);
void show_stat(int init, int end, int win);
void erase_stat(void);
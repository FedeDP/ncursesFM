#include "helper_functions.h"

#define INFO_HEIGHT 2
#define STAT_COL 30
#define MAX_FILENAME_LENGTH 25

void screen_init(void);
void screen_end(void);
void reset_win(int win);
void list_everything(int win, int old_dim, int end, char *files[]);
void new_tab(void);
void delete_tab(void);
void enlarge_first_tab(void);
void scroll_down(char **str);
void scroll_up(char **str);
void trigger_show_helper_message(void);
void show_stat(int init, int end, int win);
void show_stats(void);
int win_refresh_and_getch(void);

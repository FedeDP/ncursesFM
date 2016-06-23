#include "fm_functions.h"

void get_bookmarks(void);
void add_file_to_bookmarks(const char *str);
void remove_bookmark_from_file(void);
void show_bookmarks(void);
void manage_enter_bookmarks(struct stat current_file_stat);
void remove_all_user_bookmarks(void);

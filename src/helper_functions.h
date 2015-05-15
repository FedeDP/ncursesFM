#include "ui_functions.h"

int is_extension(const char *filename, const char **extensions);
int file_isCopied(void);
void get_full_path(char *full_path_current_position, int i, int win);
int ask_user(const char *str);
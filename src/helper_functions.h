#include "declarations.h"

int is_extension(const char *filename, const char **extensions);
int file_isCopied(const char *str);
char *ask_user(const char *str, char *input);
void print_info(const char *str, int i);
void *safe_malloc(ssize_t size, const char *str);
void free_str(char *str[PATH_MAX]);
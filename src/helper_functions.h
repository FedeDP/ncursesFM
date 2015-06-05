#include "declarations.h"
#include <magic.h>

int is_archive(const char *filename);
int file_isCopied(const char *str);
void ask_user(const char *str, char *input, int dim, char c);
void print_info(const char *str, int i);
void *safe_malloc(ssize_t size, const char *str);
void free_str(char *str[PATH_MAX]);
int get_mimetype(const char *path, const char *test);
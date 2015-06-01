#include "declarations.h"

int is_extension(const char *filename, const char **extensions);
int file_isCopied(char *str);
char *ask_user(const char *str, char *input);
char *strrstr(const char* str1, const char* str2);
void print_info(const char *str, int i);
void *safe_malloc(ssize_t size, char *str);
void free_found(void);
#include "declarations.h"
#include <sys/stat.h>
#include <magic.h>
#include <unistd.h>

int is_archive(const char *filename);
void ask_user(const char *str, char *input, int dim, char c);
void print_info(const char *str, int i);
void *safe_malloc(ssize_t size, const char *str);
void free_str(char *str[PATH_MAX]);
int get_mimetype(const char *path, const char *test);
thread_l *add_thread(thread_l *h);
void init_thread(int type, void (*f)(void), const char *str);
void free_copied_list(file_list *h);
int remove_from_list(const char *name);
file_list *select_file(char c, file_list *h, const char *str);
void free_everything(void);

#include "declarations.h"
#include <magic.h>
#include <signal.h>
#include <unistd.h>

int is_archive(const char *filename);
int file_isCopied(const char *str, int level);
void ask_user(const char *str, char *input, int dim, char c);
void print_info(const char *str, int i);
void *safe_malloc(ssize_t size, const char *str);
void free_str(char *str[PATH_MAX]);
int get_mimetype(const char *path, const char *test);
int is_thread_running(pthread_t th);
thread_l *add_thread(thread_l *h);
thread_l *free_old_thread_h(thread_l *x);
void init_thread(int type, void (*f)(void));
void free_copied_list(file_list *h);
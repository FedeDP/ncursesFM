#define _GNU_SOURCE
#include <ftw.h>
#include "quit_functions.h"
#include <pthread.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

void change_dir(char *str);
void switch_hidden(void);
void manage_file(char *str);
void open_file(char *str);
void iso_mount_service(char *str);
void new_file(void);
void remove_file(void);
void manage_c_press(char c);
int remove_from_list(char *name);
void copy_file(char c);
void paste_file(void);
void *cpr(void *x);
void check_pasted(void);
void rename_file_folders(void);
void create_dir(void);
int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
int rmrf(char *path);
int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
int search_file(char *path);
void search(void);
int search_loop(int size);
void print_support(char *str);
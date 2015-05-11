#define _GNU_SOURCE
#include <ftw.h>
#include "quit_functions.h"
#include <pthread.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <cups/cups.h>

void change_dir(char *str);
void switch_hidden(void);
void manage_file(char *str);
static void open_file(char *str);
static void mount_service(char *str, int dim);
void new_file(void);
void remove_file(void);
void manage_c_press(char c);
static int remove_from_list(char *name);
static void copy_file(char c);
void paste_file(void);
static void *cpr(void *x);
static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
void check_pasted(void);
void rename_file_folders(void);
void create_dir(void);
static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int rmrf(char *path);
static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int search_file(char *path);
void search(void);
static int search_loop(int size);
void print_support(char *str);
void *print_file(void *filename);
#define _GNU_SOURCE
#include <ftw.h>
#include "quit_functions.h"
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <cups/cups.h>
#include <archive.h>
#include <archive_entry.h>
#define MAX_NUMBER_OF_FOUND 100
#define BUFF_SIZE 8192
#define CANNOT_PASTE_SAME_DIR -2
#define MOVED_FILE -1

void change_dir(char *str);
void switch_hidden(void);
void manage_file(char *str);
static void open_file(char *str);
static void iso_mount_service(char *str, int dim);
void new_file(void);
void remove_file(void);
void manage_c_press(char c);
static int remove_from_list(char *name);
static file_list *select_file(char c, file_list *h);
void paste_file(void);
static void *cpr(void *x);
static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void check_pasted(void);
void rename_file_folders(void);
void create_dir(void);
static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int rmrf(char *path);
static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void search_inside_archive(const char *path, int i);
static int search_file(char *path);
void search(void);
static void free_found(void);
static void search_loop(int size);
void print_support(char *str);
static void *print_file(void *filename);
void create_archive(void);
static void *archiver_func(void *x);
static int recursive_archive(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void try_extractor(char *path);
static void *extractor_thread(void *a);
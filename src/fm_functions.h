#define _GNU_SOURCE
#include <ftw.h>
#include "ui_functions.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <cups/cups.h>
#include <archive.h>
#include <archive_entry.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <X11/Xlib.h>

#define MAX_NUMBER_OF_FOUND 100
#define BUFF_SIZE 8192
#define CANNOT_PASTE_SAME_DIR -2
#define MOVED_FILE -1

void change_dir(const char *str);
void switch_hidden(void);
void manage_file(const char *str);
void new_file(void);
void remove_file(void);
void manage_c_press(char c);
void paste_file(void);
void rename_file_folders(void);
void create_dir(void);
void search(void);
void list_found(void);
void search_loop(void);
void print_support(char *str);
void create_archive(void);
void integrity_check(const char *str);
void change_tab(void);
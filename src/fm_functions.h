#define _GNU_SOURCE
#include <ftw.h>
#include "ui_functions.h"
#include <sys/wait.h>
#include <sys/file.h>
#include <archive.h>
#include <archive_entry.h>
#ifdef LIBX11_PRESENT
#include <X11/Xlib.h>
#endif
#ifdef LIBCUPS_PRESENT
#include <cups/cups.h>
#endif

#define MAX_NUMBER_OF_FOUND 100
#define BUFF_SIZE 8192

void change_dir(const char *str);
void switch_hidden(void);
void manage_file(const char *str);
int new_file(void);
int remove_file(void);
void manage_space_press(const char *str);
int paste_file(void);
int move_file(void);
int rename_file_folders(void);
void search(void);
void list_found(void);
#ifdef LIBCUPS_PRESENT
void print_support(char *str);
#endif
int create_archive(void);
void change_tab(void);
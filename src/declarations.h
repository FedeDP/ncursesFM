#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>
#include "string_constants.h"

#define INITIAL_POSITION 1
#define MAX_TABS 2

#define INFO_LINE 0
#define ERR_LINE 1

#define PASTE_TH 1
#define ARCHIVER_TH 2
#define EXTRACTOR_TH 3
#define RM_TH 4
#define RENAME_TH 5

struct conf {
    char *editor;
    int show_hidden;
    char *starting_dir;
    int second_tab_starting_dir;
};

typedef struct list {
    char name[PATH_MAX];
    int cut;
    struct list *next;
} file_list;

struct vars {
    int curr_pos;
    int delta;
    int stat_active;
    char my_cwd[PATH_MAX];
    WINDOW *fm;
    char *nl[PATH_MAX];
};

struct search_vars {
    char searched_string[20];
    char *found_searched[PATH_MAX];
    int searching;
    int search_archive;
};

typedef struct thread_list {
    file_list *selected_files;
    void (*f)(void);
    char full_path[PATH_MAX];
    struct thread_list *next;
} thread_l;

thread_l *thread_h, *running_h, *current_th; // current_th: ptr to latest elem in thread_l list
pthread_t th;
struct conf config;
struct vars ps[MAX_TABS];
struct search_vars sv;
WINDOW *info_win;
int active, cont, thread_type, quit;
pthread_mutex_t lock;
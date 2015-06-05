#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define INITIAL_POSITION 1
#define MAX_TABS 2
#define INFO_LINE 0
#define ERR_LINE 1

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
    int search_active_win;
};

file_list *selected_files;
pthread_t paste_th, archiver_th, extractor_th;
struct conf config;
struct vars ps[MAX_TABS];
struct search_vars sv;
WINDOW *info_win;
int active, cont;
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

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
    char dir[PATH_MAX];
    int cut;
    struct list *next;
} file_list;

struct vars {
    int current_position;
    int number_of_files;
    int delta;
    int stat_active;
    char my_cwd[PATH_MAX];
    WINDOW *file_manager;
    struct dirent **namelist;
};

file_list *selected_files;
pthread_t th;
struct conf config;
struct vars ps[MAX_TABS];
WINDOW *info_win;
int dim, active, cont;
char *info_message;
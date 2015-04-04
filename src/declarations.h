#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#define INITIAL_POSITION 1
#define MAX_TABS 2
#define INFO_LINE 0
#define ERR_LINE 1
#define INFO_HEIGHT 2
#define HELPER_HEIGHT 14
#define STAT_COL 30
#define MAX_FILENAME_LENGTH 25

struct conf {
    char *editor;
    int show_hidden;
    char *starting_dir;
    int second_tab_starting_dir;
};

typedef struct list {
    char copied_file[PATH_MAX];
    char copied_dir[PATH_MAX];
    int cut;
    struct list *next;
} copied_file_list;

struct vars {
    int current_position;
    int number_of_files;
    int delta;
    int stat_active;
    char my_cwd[PATH_MAX];
    WINDOW *file_manager;
    struct dirent **namelist;
};

copied_file_list *copied_files;
pthread_t th;
struct conf config;
struct vars ps[MAX_TABS];
WINDOW *info_win;
int dim, pasted, active, cont;
char pasted_dir[PATH_MAX];
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
    char *iso_mount_point;
    char *starting_dir;
};

typedef struct list {
    char copied_file[PATH_MAX];
    char copied_dir[PATH_MAX];
    int cut;
    struct list *next;
} copied_file_list;

struct vars {
    int current_position[MAX_TABS];
    int number_of_files[MAX_TABS];
    int active;
    int cont;
    int delta[MAX_TABS];
    int stat_active[MAX_TABS];
    copied_file_list *copied_files;
    char pasted_dir[PATH_MAX];
    char my_cwd[MAX_TABS][PATH_MAX];
    int pasted;
};

pthread_t th;
struct conf config;
struct vars ps;
WINDOW *file_manager[MAX_TABS], *info_win;
int dim;
struct dirent **namelist[MAX_TABS];
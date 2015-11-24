#include <dirent.h>
#include <pthread.h>
#include <errno.h>

#define MAX_TABS 2
#define MAX_NUMBER_OF_FOUND 100
#define BUFF_SIZE 8192

/*
 * Lines where to print messages to user inside info_win (ui_functions)
 */
#define ASK_LINE 0
#define INFO_LINE 1
#define ERR_LINE 2

/*
 * Long operations that require a worker thread
 */
#define MOVE_TH 0
#define PASTE_TH 1
#define RM_TH 2
#define ARCHIVER_TH 3
#define EXTRACTOR_TH 4

/*
 * Short (fast) operations that do not require spawning a separate thread
 */
#define NEW_FILE_TH 0
#define CREATE_DIR_TH 1
#define RENAME_TH 2

#define DONT_QUIT 0 
#define NORM_QUIT 1
#define MEM_ERR_QUIT 2

/*
 * Just a flag to be passed to list_found_or_devices() in ui_functions.c
 */
#define SEARCH 0
#define DEVICE 1

struct conf {
    char editor[PATH_MAX];
    int show_hidden;
    char starting_dir[PATH_MAX];
    int second_tab_starting_dir;
#ifdef SYSTEMD_PRESENT
    int inhibit;
#endif
    int starting_helper;
};

typedef struct list {
    char name[PATH_MAX];
    struct list *next;
} file_list;

struct vars {
    int curr_pos;
    char my_cwd[PATH_MAX];
    char (*nl)[PATH_MAX];
    int number_of_files;
    char title[PATH_MAX];
};

struct search_vars {
    char searched_string[20];
    char found_searched[MAX_NUMBER_OF_FOUND][PATH_MAX];
    int searching;
    int search_archive;
    int found_cont;
};

typedef struct thread_list {
    file_list *selected_files;
    int (*f)(void);
    char full_path[PATH_MAX];
    char filename[PATH_MAX];
    struct thread_list *next;
    int num;
    int type;
} thread_job_list;

thread_job_list *thread_h;
file_list *selected;
struct conf config;
struct vars ps[MAX_TABS];
struct search_vars sv;
int active, quit, num_of_jobs, cont, distance_from_root;
#ifdef SYSTEMD_PRESENT 
pthread_t install_th;
#ifdef LIBUDEV_PRESENT
char (*usb_devices)[PATH_MAX];
int device_mode;
#endif
#endif
char (*str_ptr[MAX_TABS])[PATH_MAX]; // pointer to make abstract which list of strings i have to print in list_everything()
pthread_mutex_t fm_lock;
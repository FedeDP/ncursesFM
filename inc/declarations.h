#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>

#define MAX_TABS 2
#define MAX_NUMBER_OF_FOUND 100
#define MAX_BOOKMARKS 30
#define BUFF_SIZE 8192

/*
 * Lines where to print messages to user inside info_win (ui_functions)
 */
#define ASK_LINE 0
#define INFO_LINE 1
#define ERR_LINE 2
#define SYSTEM_INFO_LINE 3

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

/*
 * Quit status
 */
#define NORM_QUIT 1
#define MEM_ERR_QUIT 2

/*
 * Device monitor status
 */
#define DEVMON_OFF -2
#define DEVMON_STARTING -1
#define DEVMON_READY 0

/*
 * Useful macro to know number of elements in arrays
 */
#define NUM(a) (sizeof(a) / sizeof(*a))

/*
 * Config settings struct
 */
struct conf {
    char editor[PATH_MAX + 1];
    int show_hidden;
    char starting_dir[PATH_MAX + 1];
    int second_tab_starting_dir;
#ifdef SYSTEMD_PRESENT
    int inhibit;
#ifdef LIBUDEV_PRESENT
    int automount;
#endif
#endif
    int starting_helper;
    int loglevel;
    int persistent_log;
    int bat_low_level;
};

/*
 * List used to store file names when selecting them
 */
typedef struct list {
    char name[PATH_MAX];
    struct list *next;
} file_list;

/*
 * Struct used to store tab's information
 */
struct vars {
    int curr_pos;
    char my_cwd[PATH_MAX];
    char (*nl)[PATH_MAX + 1];
    int number_of_files;
    char title[PATH_MAX + 1];
};

/*
 * Struct used to store searches information
 */
struct search_vars {
    char searched_string[20];
    char found_searched[MAX_NUMBER_OF_FOUND][PATH_MAX + 1];
    int searching;
    int search_archive;
    int search_lazy;
    int found_cont;
};

/*
 * Struct that defines a list of thread job to be executed one after the other.
 */
typedef struct thread_list {
    file_list *selected_files;
    int (*f)(void);
    char full_path[PATH_MAX + 1];
    char filename[PATH_MAX + 1];
    struct thread_list *next;
    int num;
    int type;
} thread_job_list;

struct pollfd main_p;
sigset_t main_mask;
thread_job_list *thread_h;
file_list *selected;
struct conf config;
struct vars ps[MAX_TABS];
struct search_vars sv;
int active, quit, num_of_jobs, cont, distance_from_root;
int device_mode, special_mode[MAX_TABS], fast_browse_mode[MAX_TABS], bookmarks_mode[MAX_TABS];
pthread_mutex_t fm_lock[MAX_TABS], time_lock;
#ifdef SYSTEMD_PRESENT
pthread_t install_th;
#ifdef LIBUDEV_PRESENT
pthread_t monitor_th;
#endif
#endif
pthread_t worker_th, search_th, time_th;
char (*str_ptr[MAX_TABS])[PATH_MAX + 1]; // pointer to abstract which list of strings i have to print in list_everything()
char (bookmarks[MAX_BOOKMARKS])[PATH_MAX + 1];
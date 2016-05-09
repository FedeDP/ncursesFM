#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <sys/inotify.h>
#include <ncurses.h>

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
#define GENERIC_ERR_QUIT 3

/*
 * Device monitor status
 */
#define DEVMON_OFF -2
#define DEVMON_STARTING -1
#define DEVMON_READY 0

/*
 * Search status
 */
#define NO_SEARCH 0
#define SEARCHING 1
#define SEARCHED 2

/*
 * struct pollfd indexes
 */
#define GETCH_IX 0
#define TIMER_IX 1
#define INOTIFY_IX1 2
#define INOTIFY_IX2 3
#define INFO_IX 4
#define DEVMON_IX 5

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
    int automount;
#endif
    int starting_helper;
    int loglevel;
    int persistent_log;
    int bat_low_level;
    char border_chars[8];
    char cursor_chars[2];
};

/*
 * List used to store file names when selecting them
 */
typedef struct list {
    char name[PATH_MAX];
    struct list *next;
} file_list;

/*
 * for each tab: an fd to catch inotify events,
 * and a wd, that uniquely represents an inotify watch.
 */
struct inotify {
    int fd;
    int wd;
};

/*
 * Struct that holds UI informations per-tab
 */
struct scrstr {
    WINDOW *fm;
    int width;
    int delta;
    int stat_active;
    char tot_size[30];
};

enum working_mode {normal, fast_browse_, bookmarks_, search_, device_};

/*
 * Struct used to store tab's information
 */
struct tab {
    int curr_pos;
    char my_cwd[PATH_MAX];
    char (*nl)[PATH_MAX + 1];
    int number_of_files;
    char title[PATH_MAX + 1];
    struct inotify inot;
    char old_file[NAME_MAX + 1];
    struct scrstr mywin;
    enum working_mode mode;
    int show_hidden;
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

/*
 * main_p: Needed to interrupt main cycles getch
 * from external signals;
 * nfds: number of elements in main_p struct;
 * info_fd: pipe used to pass info_msg waiting 
 * to be printed to main_poll.
 */
struct pollfd *main_p;
sigset_t main_mask;
int nfds, info_fd[2];

thread_job_list *thread_h;
file_list *selected;
struct conf config;
struct tab ps[MAX_TABS];
struct search_vars sv;

/*
 * active win, quit status, number of worker thread jobs,
 * tabs counter and device_init status.
 */
int active, quit, num_of_jobs, cont, device_init, previewer;

#ifdef SYSTEMD_PRESENT
pthread_t install_th;
#endif
pthread_t worker_th, search_th;

/*
 * pointer to abstract which list of strings currently 
 * is active for current tab
 */
char (*str_ptr[MAX_TABS])[PATH_MAX + 1];

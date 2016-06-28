#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <ncurses.h>
#include <libintl.h>
#include <archive.h>
#include <archive_entry.h>

#include "version.h"

// used for internationalization
#define _(str) gettext(str)

#define MAX_TABS 2
#define MAX_NUMBER_OF_FOUND 100
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
 * Safe values
 */
#define FULL_SAFE 2
#define SAFE 1
#define UNSAFE 0

/*
 * struct pollfd indexes
 */
#define GETCH_IX 0
#define TIMER_IX 1
#define INOTIFY_IX1 2
#define INOTIFY_IX2 3
#define INFO_IX 4
#define SIGNAL_IX 5
#if ARCHIVE_VERSION_NUMBER >= 3002000
#define ARCHIVE_IX 6
#define DEVMON_IX 7
#else
#define DEVMON_IX 6
#endif

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
    int safe;
    wchar_t cursor_chars[3];
    char sysinfo_layout[4];
};

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

enum working_mode {normal, fast_browse_, bookmarks_, search_, device_, selected_};

/*
 * Struct used to store tab's information
 */
struct tab {
    int curr_pos;
    char my_cwd[PATH_MAX + 1];
    char (*nl)[PATH_MAX + 1];
    int number_of_files;
    char title[PATH_MAX + 1];
    struct inotify inot;
    char old_file[NAME_MAX + 1];
    struct scrstr mywin;
    enum working_mode mode;
    int show_hidden;
    int sorting_index;
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
    // list of file selected for this job
    char (*selected_files)[PATH_MAX + 1];
    // number of files selected for this job
    int num_selected;
    // function associated to this job
    int (*f)(void);
    // when needed: fullpath  (eg where to extract each file)
    char full_path[PATH_MAX + 1];
    // when needed: (only in archiving for now): filename of the archive to be created
    char filename[PATH_MAX + 1];
    // next job pointer
    struct thread_list *next;
    // num of this job (index of this job in thread_h list)
    // needed when printing "job [1/4]" on INFO_LINE
    int num;
    // type of this job (needed to associate it with its function)
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
int nfds, info_fd[2];
#if ARCHIVE_VERSION_NUMBER >= 3002000
int archive_cb_fd[2];
char passphrase[100];
#endif
thread_job_list *thread_h;
char (*selected)[PATH_MAX + 1];
struct conf config;
struct tab ps[MAX_TABS];
struct search_vars sv;

/*
 * active win, quit status, number of worker thread jobs,
 * tabs counter and device_init status.
 * Has_X-> needed for xdg-open (if we're on a X env)
 * num_selected -> number of selected files
 */
int active, quit, num_of_jobs, cont, device_init, has_X, num_selected;

#ifdef SYSTEMD_PRESENT
pthread_t install_th;
#endif
pthread_t worker_th, search_th;

/*
 * pointer to abstract which list of strings currently 
 * is active for current tab
 */
char (*str_ptr[MAX_TABS])[PATH_MAX + 1];

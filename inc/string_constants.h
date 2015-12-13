#define HELPER_HEIGHT 13
#define LONG_FILE_OPERATIONS 5
#define SHORT_FILE_OPERATIONS 3

#ifdef LIBCONFIG_PRESENT
const char *config_file_missing;
#endif
const char *editor_missing;

const char *generic_mem_error;
const char *fatal_mem_error;

const char *no_w_perm;

const char *info_win_str[3];

const char *arch_ext[6];

const char *file_selected;
const char *no_selected_files;
const char *file_sel[3];

const char *sure;

const char *big_file;

const char *already_searching;
const char *search_mem_fail;
const char *search_insert_name;
const char *search_archives;
const char *searched_string_minimum;
const char *too_many_found;
const char *no_found;
char searching_mess[4][80];

#ifdef LIBCUPS_PRESENT
const char *print_question;
const char *print_ok;
const char *print_fail;
#endif

const char *archiving_mesg;

const char *ask_name;

const char *extr_question;

const char *thread_job_mesg[LONG_FILE_OPERATIONS];
const char *thread_str[LONG_FILE_OPERATIONS];
const char *thread_fail_str[LONG_FILE_OPERATIONS];
const char *short_msg[SHORT_FILE_OPERATIONS];
const char *short_fail_msg[SHORT_FILE_OPERATIONS];
const char *selected_mess;

const char *thread_running;
const char *quit_with_running_thread;

const char *helper_string[HELPER_HEIGHT - 2];

#ifdef SYSTEMD_PRESENT
const char *bus_error;
const char *pkg_quest;
const char *install_th_wait;
const char *package_warn;
#endif
#ifdef LIBUDEV_PRESENT
const char *device_mode_str;
#endif


const char *special_mode_title;
#define HELPER_HEIGHT 14
#define FILE_OPERATIONS 8

#ifdef LIBCONFIG_PRESENT
const char *config_file_missing;
#endif
const char *editor_missing;

const char *generic_mem_error;
const char *fatal_mem_error;

const char *no_w_perm;

const char *file_selected;
const char *no_selected_files;
const char *file_sel[3];

const char *sure;

const char *already_searching;
const char *search_mem_fail;
const char *search_insert_name;
const char *search_archives;
const char *searched_string_minimum;
const char *too_many_found;
const char *no_found;
const char *search_tab_title;
const char *searching_mess[2];

#ifdef LIBCUPS_PRESENT
const char *print_question;
const char *print_ok;
const char *print_fail;
#endif

const char *archiving_mesg;

const char *ask_name;

const char *extr_question;

const char *thread_job_mesg[FILE_OPERATIONS];
const char *thread_str[FILE_OPERATIONS];
const char *thread_fail_str[FILE_OPERATIONS];
const char *selected_mess;

const char *thread_running;
const char *quit_with_running_thread;

const char *helper_string[HELPER_HEIGHT - 2];
#if defined(LIBUDEV_PRESENT) && (SYSTEMD_PRESENT)
const char *device_mode_str;
#endif
#ifdef SYSTEMD_PRESENT
const char *bus_error;

const char *pkg_quest;
const char *install_th_wait;
#endif
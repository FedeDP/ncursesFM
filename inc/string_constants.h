#define LONG_FILE_OPERATIONS 5
#define SHORT_FILE_OPERATIONS 3

#define MODES 6

extern const char yes[];
extern const char no[];

extern const int HELPER_HEIGHT[MODES];

extern const char editor_missing[];

extern const char generic_error[];

extern const char *info_win_str[3];

extern const char *arch_ext[6];

extern const char *sorting_str[4];

extern const char bookmarks_add_quest[];
extern const char bookmarks_rm_quest[];
extern const char bookmarks_xdg_err[];
extern const char bookmarks_file_err[];
extern const char bookmark_added[];
extern const char bookmarks_rm[];
extern const char no_bookmarks[];
extern const char inexistent_bookmark_quest[];
extern const char inexistent_bookmark[];
extern const char bookmark_already_present[];
extern const char bookmarks_cleared[];

extern const char no_selected_files[];
extern const char *file_sel[6];
extern const char selected_cleared[];

extern const char sure[];

extern const char already_searching[];
extern const char search_insert_name[];
extern const char search_archives[];
extern const char lazy_search[];
extern const char searched_string_minimum[];
extern const char too_many_found[];
extern const char no_found[];
extern const char already_search_mode[];
extern const char *searching_mess[2];

#ifdef LIBCUPS_PRESENT
extern const char print_question[];
extern const char print_ok[];
extern const char print_fail[];
#endif

extern const char archiving_mesg[];

extern const char ask_name[];

extern const char extr_question[];

extern const char pwd_archive[];

extern const char *thread_job_mesg[LONG_FILE_OPERATIONS];
extern const char *thread_str[LONG_FILE_OPERATIONS];
extern const char *thread_fail_str[LONG_FILE_OPERATIONS];
extern const char *short_msg[SHORT_FILE_OPERATIONS];
extern const char selected_mess[];

extern const char thread_running[];
extern const char quit_with_running_thread[];

extern const char pkg_quest[];
extern const char install_th_wait[];
extern const char install_success[];
extern const char install_failed[];
extern const char package_warn[];
extern const char device_mode_str[];
extern const char no_devices[];
extern const char dev_mounted[];
extern const char dev_unmounted[];
extern const char ext_dev_mounted[];
extern const char ext_dev_unmounted[];
extern const char device_added[];
extern const char device_removed[];
extern const char no_proc_mounts[];
extern const char polling[];
extern const char monitor_err[];
extern const char bookmarks_mode_str[];
extern const char search_mode_str[];
extern const char selected_mode_str[];

extern const char ac_online[];
extern const char power_fail[];

extern const char *insert_string[];

extern const char win_too_small[];

extern const char helper_title[];
extern const char helper_string[MODES][16][150];

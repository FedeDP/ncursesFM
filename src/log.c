#include "../inc/log.h"

static FILE *log_file;
static pthread_mutex_t log_mutex;

static void log_current_options(void);

void open_log(void) {
    const char *log_name = "ncursesfm.log";
    char log_path[PATH_MAX + 1];
    
    if (config.loglevel != NO_LOG) {
        sprintf(log_path, "%s/.%s", getpwuid(getuid())->pw_dir, log_name);
        if (config.persistent_log) {
            log_file = fopen(log_path, "a+");
        } else {
            log_file = fopen(log_path, "w");
        }
        if (log_file) {
            pthread_mutex_init(&log_mutex, NULL);
            log_current_options();
        }
    }
}

static void log_current_options(void) {
    time_t t;
    struct tm tm;
    
    t = time(NULL);
    tm = *localtime(&t);
    fprintf(log_file, "\n%d-%d-%d %02d:%02d:%02d\n\n", tm.tm_year + 1900, tm.tm_mon + 1,
            tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fprintf(log_file, "NcursesFM starting options:\n\n");
    fprintf(log_file, "* EDITOR: %s\n", config.editor);
    fprintf(log_file, "* Starting directory: %s\n", config.starting_dir);
    fprintf(log_file, "* Second tab starting dir: %d\n", config.second_tab_starting_dir);
#ifdef SYSTEMD_PRESENT
    fprintf(log_file, "* Inhibition: %d\n", config.inhibit);
#ifdef LIBUDEV_PRESENT
    fprintf(log_file, "* Automount: %d\n", config.automount);
#endif
#endif
    fprintf(log_file, "* Starting with helper window: %d\n", config.starting_helper);
    fprintf(log_file, "* Log level: %d\n", config.loglevel);
    fprintf(log_file, "* Log persistency: %d\n", config.persistent_log);
    fprintf(log_file, "* Low battery threshold: %d\n\n", config.bat_low_level);
}

void log_message(const char *filename, int lineno, const char *funcname, const char *log_msg, char type, int log_level) {
    pid_t pid;
    time_t t;
    struct tm tm;
    
    if ((log_file) && (log_level <= config.loglevel)) {
        t = time(NULL);
        tm = *localtime(&t);
        pid = syscall(SYS_gettid);
        pthread_mutex_lock(&log_mutex);
        fprintf(log_file, "(%c) thread: %d, %02d:%02d:%02d, %-50s%s:%d (%s)\n",
                type, pid, tm.tm_hour, tm.tm_min, tm.tm_sec, log_msg, filename, lineno, funcname);
        pthread_mutex_unlock(&log_mutex);
    }
}

void close_log(void) {
    if (log_file) {
        pthread_mutex_lock(&log_mutex);
        fclose(log_file);
        pthread_mutex_unlock(&log_mutex);
        pthread_mutex_destroy(&log_mutex);
    }
}
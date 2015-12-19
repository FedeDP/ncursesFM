#include "../inc/quit_functions.h"

static void free_everything(void);
static void quit_thread_func(void);
static void quit_worker_th(void);
#ifdef LIBUDEV_PRESENT
static void quit_monitor_th(void);
#endif
#ifdef SYSTEMD_PRESENT
static void quit_install_th(void);
#endif
static void quit_search_th(void);

int program_quit(void) {
    free_everything();
    screen_end();
    printf("\033c"); // clear terminal/vt after leaving program
    if (quit == MEM_ERR_QUIT) {
        printf("%s\n", generic_mem_error);
        ERROR("program exited with errors.");
        exit(EXIT_FAILURE);
    }
    INFO("program exited without errors.");
    close_log();
    exit(EXIT_SUCCESS);
}

static void free_everything(void) {
    quit_thread_func();
    if (selected) {
        free_copied_list(selected);
    }
}

static void quit_thread_func(void) {
    quit_worker_th();
#ifdef SYSTEMD_PRESENT
    quit_install_th();
#endif
#ifdef LIBUDEV_PRESENT
    quit_monitor_th();
#endif
    quit_search_th();
}

static void quit_worker_th(void) {
    char c;
    
    if (thread_h) {
        ask_user(quit_with_running_thread, &c, 1, 'y');
        if (c == 'n') {
            INFO("sending SIGUSR1 signal to worker th...");
            pthread_kill(worker_th, SIGUSR1);
        }
        pthread_join(worker_th, NULL);
        INFO("worker th exited without errors.");
    }
}

#ifdef LIBUDEV_PRESENT
static void quit_monitor_th(void) {
    if ((monitor_th) && (pthread_kill(monitor_th, 0) != ESRCH)) {
        INFO("sending SIGUSR2 signal to monitor th...");
        pthread_kill(monitor_th, SIGUSR2);
        pthread_join(monitor_th, NULL);
        INFO("monitor th exited without errors.");
    }
}
#endif

#ifdef SYSTEMD_PRESENT
static void quit_install_th(void) {
    if (install_th) {
        if (pthread_kill(install_th, 0) != ESRCH) {
            print_info(install_th_wait, INFO_LINE);
        }
        INFO("waiting for package installation to finish...");
        pthread_join(install_th, NULL);
        INFO("package installation finished. Leaving.");
    }
}
#endif

static void quit_search_th(void) {
    if (sv.searching == 1) {
        INFO("waiting for search thread to leave...");
        pthread_join(search_th, NULL);
        INFO("search th left.");
    }
}

void free_copied_list(file_list *h) {
    if (h->next)
        free_copied_list(h->next);
    free(h);
}
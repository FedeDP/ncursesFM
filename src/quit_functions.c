#include "../inc/quit_functions.h"

static void free_everything(void);
static void quit_thread_func(void);
static void quit_worker_th(void);
#ifdef SYSTEMD_PRESENT
static void quit_install_th(void);
#endif
static void quit_search_th(void);

static int sig_flag;

int program_quit(int sig_received) {
    sig_flag = sig_received;
    free_everything();
    screen_end();
    if (quit == MEM_ERR_QUIT || quit == GENERIC_ERR_QUIT) {
        fprintf(stderr, "%s\n", generic_error);
        ERROR("program exited with errors.");
        close_log();
        exit(EXIT_FAILURE);
    }
    INFO("program exited without errors.");
    close_log();
    exit(EXIT_SUCCESS);
}

static void free_everything(void) {
    quit_thread_func();
#if defined SYSTEMD_PRESENT && LIBUDEV_PRESENT
    free_device_monitor();
#endif
    free_timer();
    free(main_p);
    if (selected) {
        free_copied_list(selected);
    }
    close(inot[0].fd);
    close(inot[1].fd);
    close(info_fd[0]);
    close(info_fd[1]);
}

static void quit_thread_func(void) {
    quit_worker_th();
#ifdef SYSTEMD_PRESENT
    quit_install_th();
#endif
    quit_search_th();
}

static void quit_worker_th(void) {
    char c;
    
    if (thread_h) {
        /* 
         * if we received an external signal (SIGINT or SIGTERM), waits until worker thread ends its job list
         * or until a SIGKILL is sent to the process
         */
        if (!sig_flag) {
            ask_user(quit_with_running_thread, &c, 1, 'y');
            if (c == 'n') {
                INFO("sending SIGUSR1 signal to worker th...");
                pthread_kill(worker_th, SIGUSR1);
            }
        }
        pthread_join(worker_th, NULL);
        INFO("worker th exited without errors.");
    }
}

#ifdef SYSTEMD_PRESENT
static void quit_install_th(void) {
    int installing = 0;
    
    if (install_th) {
        if (pthread_kill(install_th, 0) != ESRCH) {
            print_info(install_th_wait, INFO_LINE);
            INFO("waiting for package installation to finish...");
            installing = 1;
        }
        pthread_join(install_th, NULL);
        if (installing) {
            INFO("package installation finished. Leaving.");
        }
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
    file_list *tmp = h;
    
    while (h) {
        h = h->next;
        free(tmp);
        tmp = h;
    }
}
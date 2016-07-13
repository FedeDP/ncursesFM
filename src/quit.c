#include "../inc/quit.h"

static void free_everything(void);
static void quit_thread_func(void);
static void quit_worker_th(void);
#ifdef SYSTEMD_PRESENT
static void quit_install_th(void);
#endif
static void quit_search_th(void);
static void close_fds(void);

int program_quit(void) {
    free_everything();
    screen_end();
    close_fds();
    quit_thread_func();
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
#ifdef SYSTEMD_PRESENT
    free_device_monitor();
#endif
    free_timer();
    free(main_p);
    if (selected) {
        free(selected);
    }
}

static void quit_thread_func(void) {
    quit_worker_th();
#ifdef SYSTEMD_PRESENT
    quit_install_th();
#endif
    quit_search_th();
}

static void quit_worker_th(void) {
    if ((worker_th) && (pthread_kill(worker_th, 0) != ESRCH)) {
        INFO(quit_with_running_thread);
        printf("%s\n", quit_with_running_thread);
        pthread_join(worker_th, NULL);
        INFO("worker th exited without errors.");
        printf("Jobs queue ended.\n");
    }
}

#ifdef SYSTEMD_PRESENT
static void quit_install_th(void) {
    if ((install_th) && (pthread_kill(install_th, 0) != ESRCH)) {
        printf("%s\n", install_th_wait);
        INFO(install_th_wait);
        pthread_join(install_th, NULL);
        printf("Installation finished.\n");
        INFO("package installation finished. Leaving.");
    }
}
#endif

static void quit_search_th(void) {
    if ((search_th) && (pthread_kill(search_th, 0) != ESRCH)) {
        INFO("waiting for search thread to leave...");
        pthread_join(search_th, NULL);
        INFO("search th left.");
    }
}

static void close_fds(void) {
    close(ps[0].inot.fd);
    close(ps[1].inot.fd);
    close(info_fd[0]);
    close(info_fd[1]);
#if ARCHIVE_VERSION_NUMBER >= 3002000
    close(archive_cb_fd[0]);
    close(archive_cb_fd[1]);
#endif
}

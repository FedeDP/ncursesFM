#include "../inc/quit_functions.h"

static void free_everything(void);
static void quit_thread_func(void);
static void quit_worker_th(void) ;

int program_quit(void) {
    free_everything();
    screen_end();
    printf("\033c"); // clear terminal/vt after leaving program
    if (quit == MEM_ERR_QUIT) {
        printf("%s\n", generic_mem_error);
        exit(EXIT_FAILURE);
    }
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
    if (install_th) {
        if (pthread_kill(install_th, 0) != ESRCH) {
            print_info(install_th_wait, INFO_LINE);
        }
        pthread_join(install_th, NULL);
    }
#endif
#ifdef LIBUDEV_PRESENT
    if (monitor_th) {
        pthread_kill(monitor_th, SIGUSR1);
        pthread_join(monitor_th, NULL);
    }
#endif
}

static void quit_worker_th(void) {
    char c;
    
    if (thread_h) {
        ask_user(quit_with_running_thread, &c, 1, 'y');
        if (c == 'n') {
            pthread_kill(worker_th, SIGUSR1);
        }
        pthread_join(worker_th, NULL);
    }
}

void free_copied_list(file_list *h) {
    if (h->next)
        free_copied_list(h->next);
    free(h);
}
#include "../inc/worker_thread.h"

struct thread_mesg {
    const char *str;
    int line;
};

static thread_job_list *add_job(thread_job_list *h, int type, int (*f)(void));
static void free_running_h(void);
static void init_thread_helper(void);
static void copy_selected_files(void);
static void *execute_thread(void *x);
static void check_refresh(void);
static int refresh_needed(const char *dir);
static void free_copied_list(file_list *h);
static void free_thread_job_list(thread_job_list *h);
static void quit_thread_func(void);
static void sig_handler(int signum);

static pthread_t worker_th;
static thread_job_list *current_th; // current_th: ptr to latest elem in thread_l list
static struct thread_mesg thread_m;

/*
 * Creates a new job object for the worker_thread appending it to the end of job's queue.
 */
static thread_job_list *add_job(thread_job_list *h, int type, int (*f)(void)) {
    if (h) {
        h->next = add_job(h->next, type, f);
    } else {
        if (!(h = malloc(sizeof(struct thread_list)))) {
            quit = MEM_ERR_QUIT;
            return NULL;
        }
        num_of_jobs++;
        h->selected_files = NULL;
        h->next = NULL;
        h->f = f;
        strcpy(h->full_path, ps[active].my_cwd);
        h->type = type;
        h->num = num_of_jobs;
        current_th = h;
    }
    return h;
}

/*
 * Deletes current job object and updates job queue.
 */
static void free_running_h(void) {
    thread_job_list *tmp = thread_h;

    thread_h = thread_h->next;
    if (tmp->selected_files)
        free_copied_list(tmp->selected_files);
    free(tmp);
    tmp = NULL;
}

void init_thread(int type, int (* const f)(void)) {
    thread_h = add_job(thread_h, type, f);
    if (quit) {
        return;
    }
    init_thread_helper();
    if (quit) {
        return;
    }
    if (num_of_jobs > 1) {
        print_info(thread_running, INFO_LINE);
    } else {
#ifdef SYSTEMD_PRESENT
        if (config.inhibit) {
            inhibit_suspend();
        }
#endif
        thread_m.str = "";
        thread_m.line = INFO_LINE;
        pthread_create(&worker_th, NULL, execute_thread, NULL);
    }
}

/*
 * Fixes some needed current_th variables.
 */
static void init_thread_helper(void) {
    char name[NAME_MAX];

    if (current_th->type == EXTRACTOR_TH) {
        strcpy(current_th->filename, ps[active].nl[ps[active].curr_pos]);
    } else {
        copy_selected_files();
        if (current_th->type == ARCHIVER_TH && !quit) {
            ask_user(archiving_mesg, name, NAME_MAX, 0);
            if (!strlen(name)) {
                strcpy(name, strrchr(current_th->selected_files->name, '/') + 1);
            }
            sprintf(current_th->filename, "%s/%s.tgz", current_th->full_path, name);
        }
    }
}

static void copy_selected_files(void) {
    file_list *tmp = selected;

    while (tmp && !quit) {
        current_th->selected_files = select_file(current_th->selected_files, tmp->name);
        tmp = tmp->next;
    }
    free_copied_list(selected);
    selected = NULL;
}

/*
 * It sticks a signal handler to the thread (that is needed if user wants to leave program while worker_thread is still running;
 * we need at least to kill thread and clear job's queue to avoid memleaks)
 * then, while job's queue isn't empty, exec job's queue head function, frees its resources, updates UI and notifies user.
 * Finally, call itself recursively.
 * When job's queue is empty, reset some vars and returns.
 */
static void *execute_thread(void *x) {
    signal(SIGUSR1, sig_handler);
    print_info(thread_m.str, thread_m.line);
    if (thread_h) {
        if (thread_h->f() == -1) {
            thread_m.str = thread_fail_str[thread_h->type];
            thread_m.line = ERR_LINE;
        } else {
            thread_m.str = thread_str[thread_h->type];
            thread_m.line = INFO_LINE;
            check_refresh();
        }
        free_running_h();
        return execute_thread(NULL);
    }
    num_of_jobs = 0;
    print_info("", ASK_LINE);
#ifdef SYSTEMD_PRESENT
    if (config.inhibit) {
        free_bus();
    }
#endif
    pthread_detach(pthread_self()); // to avoid stupid valgrind errors
    return NULL;
}

static void check_refresh(void) {
    for (int i = 0; i < cont; i++) {
        if ((thread_h->type != RM_TH && strcmp(ps[i].my_cwd, thread_h->full_path) == 0) || refresh_needed(ps[i].my_cwd)) {
            tab_refresh(i);
        }
    }
}

/*
 * RM_TH and MOVE_TH only: we need cycling through selected_files and
 * check if current selected file is isnide win's cwd (to update UI)
 */
static int refresh_needed(const char *dir) {
    file_list *tmp = thread_h->selected_files;
    
    if (thread_h->type == RM_TH || thread_h->type == MOVE_TH) {
        while (tmp) {
            if (strncmp(tmp->name, dir, strlen(dir)) == 0) {
                return 1;
            }
            tmp = tmp->next;
        }
    }
    return 0;
}

static void free_copied_list(file_list *h) {
    if (h->next)
        free_copied_list(h->next);
    free(h);
}

int remove_from_list(const char *name) {
    file_list *temp = NULL, *tmp = selected;

    if (strcmp(name, tmp->name) == 0) {
        selected = selected->next;
        free(tmp);
        return 1;
    }
    while(tmp->next) {
        if (strcmp(name, tmp->next->name) == 0) {
            temp = tmp->next;
            tmp->next = tmp->next->next;
            free(temp);
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

file_list *select_file(file_list *h, const char *str) {
    if (h) {
        h->next = select_file(h->next, str);
    } else {
        if (!(h = malloc(sizeof(struct list)))) {
            quit = MEM_ERR_QUIT;
            return NULL;
        }
        strcpy(h->name, str);
        h->next = NULL;
    }
    return h;
}

void free_everything(void) {
    quit_thread_func();
    if (selected) {
        free_copied_list(selected);
    }
#if defined (SYSTEMD_PRESENT) && (LIBUDEV_PRESENT)
    if (usb_devices) {
        free(usb_devices);
    }
#endif
}

static void quit_thread_func(void) {
    char c;

    if (thread_h) {
        ask_user(quit_with_running_thread, &c, 1, 'y');
        if (c == 'y') {
            pthread_join(worker_th, NULL);
        } else {
            pthread_kill(worker_th, SIGUSR1);
        }
    }
#ifdef SYSTEMD_PRESENT
    if (install_th) {
        if (pthread_kill(install_th, 0) != ESRCH) {
            print_info(install_th_wait, INFO_LINE);
        }
        pthread_join(install_th, NULL);
    }
#endif
}

static void free_thread_job_list(thread_job_list *h) {
    if (h->next) {
        free_thread_job_list(h->next);
    }
    if (h->selected_files)
        free_copied_list(h->selected_files);
    free(h);
}

static void sig_handler(int signum) {
    free_thread_job_list(thread_h);
    #ifdef SYSTEMD_PRESENT
    if (config.inhibit) {
        free_bus();
    }
    #endif
    pthread_exit(NULL);
}
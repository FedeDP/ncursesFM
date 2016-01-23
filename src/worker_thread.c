#include "../inc/worker_thread.h"

static thread_job_list *add_job(thread_job_list *h, int type, int (*f)(void));
static void free_running_h(void);
static void init_thread_helper(void);
static void *execute_thread(void *x);
static void free_thread_job_list(thread_job_list *h);
static void sig_handler(int signum);

static thread_job_list *current_th; // current_th: ptr to latest elem in thread_l list
static struct thread_mesg thread_m;
#ifdef SYSTEMD_PRESENT
static int inhibit_fd;
#endif

/*
 * Creates a new job object for the worker_thread appending it to the end of job's queue.
 */
static thread_job_list *add_job(thread_job_list *h, int type, int (*f)(void)) {
    if (h) {
        h->next = add_job(h->next, type, f);
    } else {
        if (!(h = malloc(sizeof(struct thread_list)))) {
            quit = MEM_ERR_QUIT;
            ERROR("could not malloc. Leaving.");
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
        INFO("job added to job's queue.");
    } else {
#ifdef SYSTEMD_PRESENT
        if (config.inhibit) {
            inhibit_fd = inhibit_suspend("Job in process...");
        }
#endif
        thread_m.str = "";
        thread_m.line = INFO_LINE;
        INFO("starting a job.");
        pthread_create(&worker_th, NULL, execute_thread, NULL);
    }
}

/*
 * Fixes some needed current_th variables.
 */
static void init_thread_helper(void) {
    char name[NAME_MAX + 1];
    int num = 1, len;

    if (current_th->type == EXTRACTOR_TH) {
        strcpy(current_th->filename, ps[active].nl[ps[active].curr_pos]);
    } else {
        current_th->selected_files = selected;
        selected = NULL;
        if (current_th->type == ARCHIVER_TH && !quit) {
            ask_user(archiving_mesg, name, NAME_MAX, 0);
            if (quit) {
                return;
            }
            if (!strlen(name)) {
                strcpy(name, strrchr(current_th->selected_files->name, '/') + 1);
            }
            /* avoid overwriting a compressed file in path if it has the same name of the archive being created there */
            len = strlen(name);
            strcat(name, ".tgz");
            while (access(name, F_OK) == 0) {
                sprintf(name + len, "%d.tgz", num);
                num++;
            }
            sprintf(current_th->filename, "%s/%s", current_th->full_path, name);
        }
        erase_selected_highlight();
    }
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
            print_info("", INFO_LINE);  // remove previous INFO_LINE message
        } else {
            thread_m.str = thread_str[thread_h->type];
            thread_m.line = INFO_LINE;
        }
        free_running_h();
        return execute_thread(NULL);
    }
    INFO("ended all queued jobs.");
    num_of_jobs = 0;
#ifdef SYSTEMD_PRESENT
    stop_inhibition(inhibit_fd);
#endif
    pthread_detach(pthread_self()); // to avoid stupid valgrind errors
    pthread_exit(NULL);
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
            ERROR("could not malloc. Leaving.");
            return NULL;
        }
        strcpy(h->name, str);
        h->next = NULL;
    }
    return h;
}

static void free_thread_job_list(thread_job_list *h) {
    thread_job_list *tmp = h;
    
    while (h) {
        h = h->next;
        if (tmp->selected_files) {
            free_copied_list(tmp->selected_files);
        }
        free(tmp);
        tmp = h;
    }
}

static void sig_handler(int signum) {
    INFO("received SIGUSR1 signal.");
    INFO("freeing job's list...");
    free_thread_job_list(thread_h);
    INFO("freed.");
#ifdef SYSTEMD_PRESENT
    stop_inhibition(inhibit_fd);
#endif
    pthread_exit(NULL);
}
#include "../inc/worker_thread.h"

static thread_job_list *add_job(thread_job_list *h, int type, int (*f)(void));
static void free_running_h(void);
static int init_thread_helper(void);
static void *execute_thread(void *x);

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
        strncpy(h->full_path, ps[active].my_cwd, PATH_MAX);
        h->num_selected = num_selected;
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
        free(tmp->selected_files);
    free(tmp);
    tmp = NULL;
}

void init_thread(int type, int (* const f)(void)) {
    if (!(thread_h = add_job(thread_h, type, f))) {
        return;
    }
    if (init_thread_helper() == -1) {
        return;
    }
    if (num_of_jobs > 1) {
        print_info(_(thread_running), INFO_LINE);
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
static int init_thread_helper(void) {
    char name[NAME_MAX + 1] = {0};
    int num = 1, len;

    if (current_th->type == ARCHIVER_TH) {
        ask_user(_(archiving_mesg), name, NAME_MAX);
        if (name[0] == 27) {
            free(current_th);
            current_th = NULL;
            return -1;
        }
        if (!strlen(name)) {
            strncpy(name, strrchr(selected[0], '/') + 1, NAME_MAX);
        }
        /* avoid overwriting a compressed file in path if it has the same name of the archive being created there */
        len = strlen(name);
        strcat(name, ".tgz");
        while (access(name, F_OK) == 0) {
            sprintf(name + len, "%d.tgz", num);
            num++;
        }
        snprintf(current_th->filename, PATH_MAX, "%s/%s", current_th->full_path, name);
    }
    current_th->selected_files = selected;
    selected = NULL;
    num_selected = 0;
    erase_selected_highlight();
    return 0;
}

/*
 * While job's queue isn't empty, exec job's queue head function, frees its resources, updates UI and notifies user.
 * Finally, call itself recursively.
 * When job's queue is empty, reset some vars and returns.
 */
static void *execute_thread(void *x) {
    if (thread_h) {
        if (thread_h->f() == -1) {
            thread_m.str = thread_fail_str[thread_h->type];
            ERROR(thread_fail_str[thread_h->type]);
            thread_m.line = ERR_LINE;
            print_info("", INFO_LINE);  // remove previous INFO_LINE message
        } else {
            thread_m.str = thread_str[thread_h->type];
            INFO(thread_str[thread_h->type]);
            thread_m.line = INFO_LINE;
        }
        free_running_h();
        print_info(_(thread_m.str), thread_m.line);
        return execute_thread(NULL);
    }
    INFO("ended all queued jobs.");
    num_of_jobs = 0;
#ifdef SYSTEMD_PRESENT
    stop_inhibition(inhibit_fd);
#endif
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

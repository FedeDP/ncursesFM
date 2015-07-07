/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * NcursesFM: file manager in C with ncurses UI for linux.
 * https://github.com/FedeDP/ncursesFM
 *
 * Copyright (C) 2015  Federico Di Pierro <nierro92@gmail.com>
 *
 * This file is part of ncursesFM.
 * ncursesFM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "helper_functions.h"

static void free_running_h(void);
static void init_thread_helper(const char *temp, const char *str);
static void copy_selected_files(void);
static void *execute_thread(void *x);
static void free_thread_job_list(thread_job_list *h);
static void quit_thread_func(void);

static int num_of_jobs = 0;
static pthread_t th;
static thread_job_list *current_th; // current_th: ptr to latest elem in thread_l list

/*
 * Given a filename, the function checks:
 * 1) if there is a '.' starting substring in it (else it returns 0)
 * 2) for each of the *ext strings, checks if last "strlen(*ext)" chars in filename are equals to *ext. If yes, returns 1.
 */
int is_archive(const char *filename)
{
    const char *ext[] = {".tgz", ".tar.gz", ".zip", ".rar", ".xz", ".ar"};
    int i = 0, len = strlen(filename);

    if (strrchr(filename, '.')) {
        while (i < 6) {
            if (strcmp(filename + len - strlen(ext[i]), ext[i]) == 0) {
                return 1;
            }
            i++;
        }
    }
    return 0;
}

/*
 * Given a str, a char input[dim], and a char c (that is default value if enter is pressed, if dim == 1),
 * asks the user "str" and saves in input the user response.
 */
void ask_user(const char *str, char *input, int dim, char c)
{
    echo();
    print_info(str, INFO_LINE);
    if (dim == 1) {
        *input = wgetch(info_win);
        if ((*input >= 'A') && (*input <= 'Z')) {
            *input = tolower(*input);
        } else if (*input == 10) {
            *input = c;
        }
    } else {
        wgetstr(info_win, input);
    }
    noecho();
    print_info(NULL, INFO_LINE);
}

/*
 * Given a string str, and a line i, prints str on the I line of INFO_WIN.
 * Plus, it searches for running th, and if found, prints running_h message(depends on its type) at the end of INFO_LINE
 * It searches for selected_files too, and prints a message at the end of INFO_LINE - (strlen(running_h mesg) if there's.
 * Finally, if a search is running, prints a message at the end of ERR_LINE;
 */
void print_info(const char *str, int i)
{
    int k;
    char st[PATH_MAX];

    for (k = INFO_LINE; k != ERR_LINE + 1; k++) {
        wmove(info_win, k, strlen("I:") + 1);
        wclrtoeol(info_win);
    }
    k = 0;
    if (running_h && running_h->type) {
        sprintf(st, "[%d/%d] %s", running_h->num, num_of_jobs, thread_job_mesg[running_h->type - 1]);
        k = strlen(st) + 1;
        mvwprintw(info_win, INFO_LINE, COLS - strlen(st), st);
    }
    if (selected) {
        mvwprintw(info_win, INFO_LINE, COLS - k - strlen(selected_mess), selected_mess);
    }
    if ((sv.searching == 1) || (sv.searching == 2)) {
        mvwprintw(info_win, ERR_LINE, COLS - strlen(searching_mess[sv.searching - 1]), searching_mess[sv.searching - 1]);
    }
    if (str) {
        mvwprintw(info_win, i, strlen("I: ") + 1, str);
    }
    wrefresh(info_win);
}

void *safe_malloc(ssize_t size, const char *str)
{
    void *ptr = NULL;

    if (!(ptr = malloc(size))) {
        print_info(str, ERR_LINE);
    }
    return ptr;
}

void free_str(char *str[PATH_MAX])
{
    int i;

    for (i = 0; str[i]; i++) {
        str[i] = realloc(str[i], 0);
        free(str[i]);
    }
}

/*
 * Gived a full path: if test != NULL, searches the string test in the mimetype, and if found returns 1;
 * If !test, it just prints the mimetype.
 */
int get_mimetype(const char *path, const char *test)
{
    int ret = 0;
    const char *mimetype;
    magic_t magic_cookie;

    magic_cookie = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic_cookie, NULL);
    mimetype = magic_file(magic_cookie, path);
    if (test) {
        if (strstr(mimetype, test)) {
            ret = 1;
        }
    } else {
        print_info(mimetype, INFO_LINE);
    }
    magic_close(magic_cookie);
    return ret;
}

/*
 * Adds a job to the thread_job_list (thread_job_list list).
 * current_th will always point to the newly created job (ie the last job to be executed).
 */
thread_job_list *add_thread(thread_job_list *h)
{
    if (h) {
        h->next = add_thread(h->next);
    } else {
        if (!(h = safe_malloc(sizeof(struct thread_list), generic_mem_error))) {
            return NULL;
        }
        h->selected_files = NULL;
        h->next = NULL;
        h->f = NULL;
        current_th = h;
        num_of_jobs++;
    }
    return h;
}

/*
 * running_h is a ptr to the currently running job.
 * This function will free its memory and set it to NULL.
 */
void free_running_h(void)
{
    if (running_h->selected_files)
        free_copied_list(running_h->selected_files);
    free(running_h);
    running_h = NULL;
}

/*
 * Given a type, a function and a str:
 * - checks if we have write oaccess in cwd.
 * - if type is one of {RENAME_TH, NEW_FILE_TH, CREATE_DIR_TH}, asks user the name of the new file.
 * - adds a new job to the thread_job_list
 * - Initializes the new job
 * - if there's a thread running, prints a message that the job will be queued, else starts the th to execute the job.
 */
void init_thread(int type, void (*f)(void), const char *str)
{
    char temp[PATH_MAX];

    if (access(ps[active].my_cwd, W_OK) != 0) {
        print_info(no_w_perm, ERR_LINE);
        return;
    }
    if (type >= RENAME_TH) {
        ask_user(ask_name, temp, PATH_MAX, 0);
        if (!strlen(temp)) {
            return;
        }
    }
    thread_h = add_thread(thread_h);
    strcpy(current_th->full_path, str);
    current_th->num = num_of_jobs;
    current_th->type = type;
    init_thread_helper(temp, str);
    current_th->f = f;
    if (running_h) {
        print_info(thread_running, INFO_LINE);
    } else {
        thread_m.str = NULL;
        pthread_create(&th, NULL, execute_thread, NULL);
    }
}

/*
 * Just a helper thread for init_thread(); it sets some members of thread_job_list struct depending of current_th->type
 */
static void init_thread_helper(const char *temp, const char *str)
{
    char name[PATH_MAX];

    switch (current_th->type) {
    case RENAME_TH:
        strcpy(name, ps[active].my_cwd);
        sprintf(name + strlen(name), "/%s", temp);
        current_th->selected_files = select_file(0, current_th->selected_files, name);
        break;
    case PASTE_TH: case ARCHIVER_TH:
        copy_selected_files();
        if (current_th->type == ARCHIVER_TH) {
            ask_user(archiving_mesg, name, PATH_MAX, 0);
            if (!strlen(name)) {
                strcpy(name, strrchr(current_th->selected_files->name, '/') + 1);
            }
            sprintf(current_th->full_path + strlen(str), "/%s.tgz", name);
        }
        break;
    case NEW_FILE_TH: case CREATE_DIR_TH:
        strcat(current_th->full_path, "/");
        strcat(current_th->full_path, temp);
        break;
    }
}

static void copy_selected_files(void)
{
    file_list *tmp = selected;

    while (tmp) {
        current_th->selected_files = select_file(tmp->cut, current_th->selected_files, tmp->name);
        tmp = tmp->next;
    }
    free_copied_list(selected);
    selected = NULL;
}

/*
 * - first check if we come from a recursive call (if we're in a running th), then frees previously finished job and print its completion mesg.
 * - if thread_h is not NULL, and thread_f is already assigned, the th has another job waiting for it, so we run it, else we finished our work: num_of_jobs = 0.
 */
static void *execute_thread(void *x)
{
    if (running_h) {
        free_running_h();
        print_info(thread_m.str, thread_m.line);
    }
    if (thread_h && thread_h->f) {
        running_h = thread_h;
        thread_h = thread_h->next;
        running_h->f();
        execute_thread(NULL);
    } else {
        num_of_jobs = 0;
    }
    return NULL;
}

void free_copied_list(file_list *h)
{
    if (h->next)
        free_copied_list(h->next);
    free(h);
}

int remove_from_list(const char *name)
{
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

file_list *select_file(char c, file_list *h, const char *str)
{
    if (h) {
        h->next = select_file(c, h->next, str);
    } else {
        if (!(h = safe_malloc(sizeof(struct list), generic_mem_error))) {
            return NULL;
        }
        strcpy(h->name, str);
        if (c == 'x') {
            h->cut = 1;
        } else {
            h->cut = 0;
        }
        h->next = NULL;
    }
    return h;
}

void free_everything(void)
{
    int j;

    if (thread_h) {
        free_thread_job_list(thread_h);
        thread_h = NULL;
    }
    quit_thread_func();
    free_str(sv.found_searched);
    for (j = 0; j < cont; j++) {
        free_str(ps[j].nl);
    }
    free(config.editor);
    free(config.starting_dir);
    if (selected) {
        free_copied_list(selected);
    }
}

void quit_thread_func(void)
{
    char c;

    if (running_h) {
        ask_user(quit_with_running_thread, &c, 1, 'y');
        if (c == 'y') {
            pthread_join(th, NULL);
        }
    }
}

void free_thread_job_list(thread_job_list *h)
{
    if (h->next) {
        free_thread_job_list(h->next);
    }
    if (h->selected_files)
        free_copied_list(h->selected_files);
    free(h);
}
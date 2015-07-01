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

static int num_of_jobs = 0;
static pthread_t th;

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

void print_info(const char *str, int i)
{
    int k;
    char st[PATH_MAX];

    for (k = INFO_LINE; k != ERR_LINE + 1; k++) {
        wmove(info_win, k, strlen("I:") + 1);
        wclrtoeol(info_win);
    }
    k = -1;
    if (running_h && running_h->type) {
        sprintf(st, "[%d/%d] %s", running_h->num, num_of_jobs, thread_job_mesg[running_h->type - 1]);
        k = strlen(st);
        mvwprintw(info_win, INFO_LINE, COLS - strlen(st), st);
    }
    if (current_th && current_th->selected_files) {
        mvwprintw(info_win, INFO_LINE, COLS - k - 1 - strlen(selected_mess), selected_mess);
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
        return NULL;
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

thread_l *add_thread(thread_l *h)
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

void free_running_h(void)
{
    if (running_h->selected_files)
        free_copied_list(running_h->selected_files);
    free(running_h);
    running_h = NULL;
}

void init_thread(int type, void *(*f)(void *), const char *str)
{
    char name[PATH_MAX], temp[PATH_MAX];
    const char *mesg = "Are you serious? y/N:> ";
    const char *name_mesg = "Insert new file name:> ";
    char c = 'n';

    temp[0] = '\0';
    if (access(ps[active].my_cwd, W_OK) != 0) {
        print_info(no_w_perm, ERR_LINE);
        return;
    }
    if (type >= EXTRACTOR_TH) {
        if (type == RM_TH) {
            ask_user(mesg, &c, 1, 'n');
        } else if (type == EXTRACTOR_TH) {
            ask_user(extr_question, &c, 1, 'y');
        } else if (type >= RENAME_TH) {
            ask_user(name_mesg, temp, PATH_MAX, 0);
        }
        if ((c != 'y') && (strlen(temp) == 0)) {
            return;
        }
        thread_h = add_thread(thread_h);
    }
    strcpy(current_th->full_path, str);
    current_th->num = num_of_jobs;
    current_th->type = type;
    if (type == RENAME_TH) {
        strcpy(name, ps[active].my_cwd);
        sprintf(name + strlen(name), "/%s", temp);
        current_th->selected_files = select_file('c', current_th->selected_files, name);
    } else if (type == ARCHIVER_TH) {
        ask_user(archiving_mesg, name, PATH_MAX, 0);
        if (!strlen(name)) {
            strcpy(name, strrchr(current_th->selected_files->name, '/') + 1);
        }
        sprintf(current_th->full_path + strlen(str), "/%s.tgz", name);
    } else if (type >= NEW_FILE_TH) {
        strcat(current_th->full_path, "/");
        strcat(current_th->full_path, temp);
    }
    current_th->f = f;
    current_th = NULL;
    if (running_h) {
        print_info(thread_running, INFO_LINE);
    } else {
        execute_thread(NULL, INFO_LINE);
    }
}

void execute_thread(const char *str, int line)
{
    int inside_th = 0;

    if (running_h) {
        free_running_h();
        inside_th = 1;
        print_info(str, line);
    }
    if (thread_h && thread_h->f) {
        running_h = thread_h;
        thread_h = thread_h->next;
        if (inside_th) {
            running_h->f(NULL);
        } else {
            pthread_create(&th, NULL, running_h->f, NULL);
        }
    } else {
        num_of_jobs = 0;
    }
}

void free_copied_list(file_list *h)
{
    if (h->next)
        free_copied_list(h->next);
    free(h);
}

int remove_from_list(const char *name)
{
    file_list *temp = NULL, *tmp = current_th->selected_files;

    if (strcmp(name, tmp->name) == 0) {
        current_th->selected_files = current_th->selected_files->next;
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

    free_str(sv.found_searched);
    for (j = 0; j < cont; j++) {
        free_str(ps[j].nl);
    }
    free(config.editor);
    free(config.starting_dir);
    if (thread_h)
        free_thread_list(thread_h);
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

void free_thread_list(thread_l *h)
{
    if (h->next) {
        free_thread_list(h->next);
    }
    if (h->selected_files)
        free_copied_list(h->selected_files);
    free(h);
}
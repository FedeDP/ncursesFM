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

int file_is_used(const char *str)
{
    file_list *tmp;

    if (running_h) {
        pthread_mutex_lock(&lock);
        tmp = running_h->selected_files;
        while (tmp) {
            if (strncmp(str, tmp->name, strlen(tmp->name)) == 0) {
                print_info(file_used_by_thread, ERR_LINE);
                pthread_mutex_unlock(&lock);
                return 1;
            }
            tmp = tmp->next;
        }
        pthread_mutex_unlock(&lock);
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
    int k, cols = COLS;

    for (k = INFO_LINE; k != ERR_LINE + 1; k++) {
        wmove(info_win, k, strlen("I:") + 1);
        wclrtoeol(info_win);
    }
    if (thread_type) {
        mvwprintw(info_win, INFO_LINE, COLS - strlen(thread_job_mesg[thread_type - 1]), thread_job_mesg[thread_type - 1]);
    }
    if (thread_h && thread_h->selected_files) {
        if (thread_type) {
            cols = COLS - strlen(thread_job_mesg[thread_type - 1]) - 1;
        }
        mvwprintw(info_win, INFO_LINE, cols - strlen(selected_mess), selected_mess);
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

int is_thread_running(void)
{
    if ((th) && (pthread_kill(th, 0) != ESRCH))
        return 1;
    return 0;
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
    }
    return h;
}

void free_running_h(void)
{
    if (running_h) {
        pthread_mutex_lock(&lock);
        if (running_h->selected_files)
            free_copied_list(running_h->selected_files);
        free(running_h);
        running_h = NULL;
        pthread_mutex_unlock(&lock);
    }
}

void init_thread(int type, void (*f)(void), const char *str)
{
    char name[PATH_MAX], temp[PATH_MAX];
    const char *mesg = "Are you serious? y/N:> ";
    const char *rename_mesg = "Insert new file name:> ";
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
        } else if (type == RENAME_TH) {
            ask_user(rename_mesg, temp, PATH_MAX, 0);
        }
        if ((c != 'y') && (strlen(temp) == 0)) {
            return;
        }
        thread_h = add_thread(thread_h);
    }
    strcpy(current_th->full_path, str);
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
    }
    current_th->f = f;
    if (is_thread_running()) {
        print_info(thread_running, INFO_LINE);
    } else {
        execute_thread();
    }
    current_th = NULL;
}

void execute_thread(void)
{
    free_running_h();
    if (thread_h && thread_h->f) {
        running_h = thread_h;
        thread_h = thread_h->next;
        running_h->f();
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
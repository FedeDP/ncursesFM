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

int file_isCopied(const char *str, int level)
{
    file_list *tmp;
    thread_l *temp = thread_h;

    while (temp && level) {
        if (level == 1) {
            pthread_mutex_lock(&lock);
            temp = thread_h;
        }
        tmp = temp->selected_files;
        while (tmp) {
            if (strncmp(str, tmp->name, strlen(tmp->name)) == 0) {
                print_info("This file is already selected for copy.", INFO_LINE);
                return 1;
            }
            tmp = tmp->next;
        }
        if (level == 1) {
            pthread_mutex_unlock(&lock);
        }
        temp = temp->next;
        level--;
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
    const char *extracting_mess = "Extracting...", *searching_mess = "Searching...";
    const char *pasting_mess = "Pasting...", *archiving_mess = "Archiving...";
    const char *found_searched_mess = "Search finished. Press f anytime to view the results.";
    const char *selected_mess = "There are selected files.";
    int mess_line, search_mess_col = COLS - strlen(searching_mess);

    for (mess_line = INFO_LINE; mess_line != ERR_LINE + 1; mess_line++) {
        wmove(info_win, mess_line, strlen("I:") + 1);
        wclrtoeol(info_win);
    }
    mess_line = INFO_LINE;
    if (thread_h->selected_files) {
        if (is_thread_running(th)) {
            if (thread_h->type == PASTE_TH) {
                mvwprintw(info_win, mess_line, COLS - strlen(pasting_mess), pasting_mess);
            } else {
                mvwprintw(info_win, mess_line, COLS - strlen(archiving_mess), archiving_mess);
            }
        } else {
            mvwprintw(info_win, mess_line, COLS - strlen(selected_mess), selected_mess);
        }
        mess_line++;
    }
    if (extracting == 1) {
        mvwprintw(info_win, mess_line, COLS - strlen(extracting_mess), extracting_mess);
        if (mess_line == INFO_LINE) {
            mess_line++;
        } else {
            search_mess_col = search_mess_col - (strlen(extracting_mess) + 1);
        }
    }
    if (sv.searching == 1) {
        mvwprintw(info_win, mess_line, search_mess_col, searching_mess);
    } else if (sv.searching == 2) {
        mvwprintw(info_win, mess_line, COLS - strlen(found_searched_mess), found_searched_mess);
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

int is_thread_running(pthread_t th)
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
        if (!(h = safe_malloc(sizeof(struct thread_list), "No memory available"))) {
            return NULL;
        }
        h->selected_files = NULL;
        h->next = NULL;
        h->type = 0;
        current_th = h;
    }
    return h;
}

void execute_thread(void)
{
    switch (thread_h->type) {
    case PASTE_TH:
        paste_file();
        break;
    case ARCHIVER_TH:
        create_archive();
        break;
    }
}

void change_thread_list_head(void)
{
    thread_l *tmp = thread_h;
    pthread_mutex_lock(&lock);
    thread_h = thread_h->next;
    pthread_mutex_unlock(&lock);
    free_copied_list(tmp->selected_files);
    free(tmp);
    if (thread_h->type != 0)
        execute_thread();
}

void init_thread(int type)
{
    const char *thread_running = "There's already a thread working on this file list. This thread will be queued.";
    const char *arch_mesg = "Insert new file name (defaults to first entry name):> ";
    char str[PATH_MAX];

    if (access(ps[active].my_cwd, W_OK) != 0) {
        print_info("Cannot paste here, check user permissions. Paste somewhere else please.", ERR_LINE);
        return;
    }
    current_th->type = type;
    switch (current_th->type) {
        case PASTE_TH:
            strcpy(current_th->full_path, ps[active].my_cwd);
            break;
        case ARCHIVER_TH:
            ask_user(arch_mesg, str, PATH_MAX, 0);
            if (!strlen(str)) {
                strcpy(str, strrchr(thread_h->selected_files->name, '/') + 1);
            }
            sprintf(current_th->full_path, "%s/%s.tgz", ps[active].my_cwd, str);
            break;
    }
    thread_h = add_thread(thread_h);
    if (is_thread_running(th)) {
        print_info(thread_running, INFO_LINE);
    } else {
        execute_thread();
    }
}

void free_copied_list(file_list *h)
{
    if (h->next)
        free_copied_list(h->next);
    free(h);
}
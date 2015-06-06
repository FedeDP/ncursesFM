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
            if (strcmp(filename + len - strlen(ext[i]), ext[i]) == 0)
                return 1;
            i++;
        }
    }
    return 0;
}

int file_isCopied(const char *str)
{
    char name[PATH_MAX];
    file_list *tmp = selected_files;
    sprintf(name, "%s/%s", ps[active].my_cwd, str);
    while (tmp) {
        if (strcmp(name, tmp->name) == 0) {
            print_info("This file is already selected for copy.", INFO_LINE);
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

void ask_user(const char *str, char *input, int dim, char c)
{
    echo();
    print_info(str, INFO_LINE);
    if (dim == 1) {
        *input = wgetch(info_win);
        if (*input == 10)
            *input = c;
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
    int mess_line = INFO_LINE, j, search_mess_col = COLS - strlen(searching_mess);
    for (j = INFO_LINE; j < 2; j++) {
        wmove(info_win, j, strlen("I:") + 1);
        wclrtoeol(info_win);
    }
    if (selected_files) {
        if ((paste_th) && (pthread_kill(paste_th, 0) != ESRCH)) {
            mvwprintw(info_win, mess_line, COLS - strlen(pasting_mess), pasting_mess);;
        } else {
            if ((archiver_th) && (pthread_kill(archiver_th, 0) != ESRCH))
                mvwprintw(info_win, mess_line, COLS - strlen(archiving_mess), archiving_mess);
            else
                mvwprintw(info_win, mess_line, COLS - strlen(selected_mess), selected_mess);
        }
        mess_line++;
    }
    if ((extractor_th) && (pthread_kill(extractor_th, 0) != ESRCH)) {
        mvwprintw(info_win, mess_line, COLS - strlen(extracting_mess), extracting_mess);
        if (mess_line == INFO_LINE)
            mess_line++;
        else
            search_mess_col = search_mess_col - (strlen(extracting_mess) + 1);
    }
    if (sv.searching == 1) {
        mvwprintw(info_win, mess_line, search_mess_col, searching_mess);
    } else {
        if (sv.searching == 2)
            mvwprintw(info_win, mess_line, COLS - strlen(found_searched_mess), found_searched_mess);
    }
    if (str)
        mvwprintw(info_win, i, strlen("I: ") + 1, str);
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
        if (strstr(mimetype, test))
            ret = 1;
    } else {
        print_info(mimetype, INFO_LINE);
    }
    magic_close(magic_cookie);
    return ret;
}
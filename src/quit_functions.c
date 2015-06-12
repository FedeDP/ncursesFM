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

#include "quit_functions.h"

void free_everything(void)
{
    int j;
    thread_l *tmp = thread_h;
    free_str(sv.found_searched);
    for (j = 0; j < cont; j++)
        free_str(ps[j].nl);
    free(config.editor);
    free(config.starting_dir);
    while (tmp) {
        if (tmp->selected_files)
            free_copied_list(tmp->selected_files);
        tmp = tmp->next;
    }
    free_thread_list(thread_h);
}

void quit_thread_func(void)
{
    char *mesg = "A thread is still running. Do you want to wait for it?(You should!) Y/n:> ";
    char c;
    while (is_thread_running(th)) {
        ask_user(mesg, &c, 1, 'y');
        if (c == 'y')
            pthread_join(th, NULL);
    }
    if (is_thread_running(extractor_th)) {
        ask_user(mesg, &c, 1, 'y');
        if (c == 'y')
            pthread_join(extractor_th, NULL);
    }
}

void free_thread_list(thread_l *h)
{
    if (h->next)
        free_thread_list(h->next);
    free(h);
}
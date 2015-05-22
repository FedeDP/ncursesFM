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

void free_copied_list(file_list *h)
{
    if (h->next)
        free_copied_list(h->next);
    free(h);
}

void free_everything(void)
{
    int i, j;
    for (j = 0; j < cont; j++) {
        if (ps[j].nl != NULL) {
            for (i = ps[j].number_of_files - 1; i >= 0; i--)
                free(ps[j].nl[i]);
            free(ps[j].nl);
        }
    }
    free(config.editor);
    free(config.starting_dir);
    if (selected_files)
        free_copied_list(selected_files);
}

void quit_func(void)
{
    char *mesg = "A thread is still running. Do you want to wait for it?(You should!) y/n:> ";
    if ((th) && (pthread_kill(th, 0) != ESRCH) && (ask_user(mesg) == 1))
        pthread_join(th, NULL);
    if ((extractor_th) && (pthread_kill(extractor_th, 0) != ESRCH) && (ask_user(mesg) == 1))
        pthread_join(extractor_th, NULL);
}
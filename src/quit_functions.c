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

void free_copied_list(copied_file_list *h)
{
    if (h->next)
        free_copied_list(h->next);
    free(h);
}

void free_everything(void)
{
    int i, j;
    for (j = 0; j < ps.cont; j++) {
        if (namelist[j] != NULL) {
            for (i = ps.number_of_files[j] - 1; i >= 0; i--)
                free(namelist[j][i]);
            free(namelist[j]);
        }
    }
    free(config.editor);
    free(config.iso_mount_point);
    if (ps.copied_files)
        free_copied_list(ps.copied_files);
}


void quit_func(void)
{
    char *mesg = "A paste job is still running. Do you want to wait for it?(You should!) y/n:> ";
    if ((ps.pasted == - 1) && (ask_user(mesg) == 1))
        pthread_join(th, NULL);
}
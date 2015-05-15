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

int is_extension(const char *filename, const char **extensions)
{
    int i = 0;
    char *str;
    while (*(extensions + i)) {
        str = strstr(filename, *(extensions + i));
        if ((str) && (strlen(str) == strlen(*(extensions + i))))
            return strlen(str);
        i++;
    }
    return 0;
}

int file_isCopied(void)
{
    char full_path_current_position[PATH_MAX];
    file_list *tmp = selected_files;
    get_full_path(full_path_current_position, ps[active].current_position, active);
    while (tmp) {
        if (strcmp(tmp->name, full_path_current_position) == 0) {
            print_info("The file is already selected for copy. Please cancel the copy before.", ERR_LINE);
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

void get_full_path(char *full_path_current_position, int i, int win)
{
    strcpy(full_path_current_position, ps[win].my_cwd);
    strcat(full_path_current_position, "/");
    strcat(full_path_current_position, ps[win].namelist[i]->d_name);
}

int ask_user(const char *str)
{
    char c;
    echo();
    print_info(str, INFO_LINE);
    c = wgetch(info_win);
    noecho();
    print_info(NULL, INFO_LINE);
    if (c == 'y')
        return 1;
    return 0;
}

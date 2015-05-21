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
        str = strrstr(filename, *(extensions + i));
        if ((str) && (strlen(str) == strlen(*(extensions + i))))
            return strlen(str);
        i++;
    }
    return 0;
}

int file_isCopied(char *str)
{
    char *name;
    file_list *tmp = selected_files;
    while (tmp) {
        name = strrstr(tmp->name, str);
        if ((name) && (strlen(str) == strlen(name))) {
            print_info("The file is already selected for copy. Please cancel the copy before.", ERR_LINE);
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
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

char *strrstr(const char* str1, const char* str2)
{
    char *strp;
    int len1, len2 = strlen(str2);
    if (len2 == 0)
        return (char *)str1;
    len1 = strlen(str1);
    if(len2 >= len1)
        return NULL;
    strp = (char *)(str1 + len1 - len2);
    while (strp != str1) {
        if (*strp == *str2) {
            if (strncmp(strp, str2, len2) == 0)
                return strp;
        }
        strp--;
    }
    return NULL;
}

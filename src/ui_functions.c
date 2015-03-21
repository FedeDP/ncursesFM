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

#include "ui_functions.h"

WINDOW *helper_win = NULL;

void screen_init(void)
{
    initscr();
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    raw();
    noecho();
    curs_set(0);
    dim = LINES - INFO_HEIGHT;
    file_manager[ps.active] = subwin(stdscr, dim, COLS, 0, 0);
    scrollok(file_manager[ps.active], TRUE);
    idlok(file_manager[ps.active], TRUE);
    info_win = subwin(stdscr, INFO_HEIGHT, COLS, dim, 0);
    keypad(file_manager[ps.active], TRUE);
}

void screen_end(void)
{
    int i;
    for (i = 0; i < ps.cont; i++) {
        wclear(file_manager[i]);
        delwin(file_manager[i]);
    }
    if (helper_win) {
        wclear(helper_win);
        delwin(helper_win);
    }
    wclear(info_win);
    delwin(info_win);
    endwin();
    delwin(stdscr);
}

void list_everything(int win, int old_dim, int end, int erase, int reset)
{
    int i;
    chdir(ps.my_cwd[win]);
    if (erase == 1) {
        for (i = 0; i < ps.number_of_files[win]; i++)
            free(namelist[win][i]);
        free(namelist[win]);
        namelist[win] = NULL;
        wclear(file_manager[win]);
        if (reset == 1) {
            ps.delta[win] = 0;
            ps.current_position[win] = 0;
        }
        ps.number_of_files[win] = scandir(ps.my_cwd[win], &namelist[win], is_hidden, alphasort);
        my_sort(win);
    }

    wborder(file_manager[win], '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(file_manager[win], 0, 0, "Current:%.*s", COLS / ps.cont - 1 - strlen("Current:"), ps.my_cwd[win]);
    for (i = old_dim; (i < ps.number_of_files[win]) && (i  < old_dim + end); i++) {
        colored_folders(i, win);
        mvwprintw(file_manager[win], INITIAL_POSITION + i - ps.delta[win], 4, "%.*s", MAX_FILENAME_LENGTH, namelist[win][i]->d_name);
        wattroff(file_manager[win], COLOR_PAIR(1));
        wattroff(file_manager[win], A_BOLD);
    }
    mvwprintw(file_manager[win], INITIAL_POSITION + ps.current_position[win] - ps.delta[win], 1, "->");
    if (ps.stat_active[win] == 1)
        show_stat(old_dim, end, win);
    wrefresh(file_manager[win]);
}

static int is_hidden(const struct dirent *current_file)
{
    if ((strlen(current_file->d_name) == 1) && (current_file->d_name[0] == '.'))
        return (FALSE);
    if (config.show_hidden == 0) {
        if ((strlen(current_file->d_name) > 1) && (current_file->d_name[0] == '.') && (current_file->d_name[1] != '.'))
            return (FALSE);
        return (TRUE);
    }
    return (TRUE);
}

static void my_sort(int win)
{
    struct dirent *temp;
    int i, j;
    for (i = 0; i < ps.number_of_files[win] - 1; i++) {
        if (namelist[win][i]->d_type !=  DT_DIR) {
            for (j = i + 1; j < ps.number_of_files[win]; j++) {
                if (namelist[win][j]->d_type == DT_DIR) {
                    temp = namelist[win][j];
                    namelist[win][j] = namelist[win][i];
                    namelist[win][i] = temp;
                    break;
                }
            }
        }
    }
}

void new_tab(void)
{
    ps.cont++;
    wresize(file_manager[ps.active], dim, COLS / ps.cont);
    wborder(file_manager[ps.active], '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(file_manager[ps.active], 0, 0, "Current:%.*s", COLS / ps.cont - 1 - strlen("Current:"), ps.my_cwd[ps.active]);
    wrefresh(file_manager[ps.active]);
    ps.active = 1;
    file_manager[ps.active] = subwin(stdscr, dim, COLS / ps.cont, 0, COLS / ps.cont);
    keypad(file_manager[ps.active], TRUE);
    scrollok(file_manager[ps.active], TRUE);
    idlok(file_manager[ps.active], TRUE);
    getcwd(ps.my_cwd[ps.active], PATH_MAX);
    list_everything(ps.active, 0, dim - 2, 1, 1);
}

void delete_tab(void)
{
    int i;
    ps.cont--;
    wclear(file_manager[ps.active]);
    delwin(file_manager[ps.active]);
    file_manager[ps.active] = NULL;
    for (i = ps.number_of_files[ps.active] - 1; i >= 0; i--)
        free(namelist[ps.active][i]);
    free(namelist[ps.active]);
    namelist[ps.active] = NULL;
    ps.number_of_files[ps.active] = 0;
    ps.stat_active[ps.active] = 0;
    memset(ps.my_cwd[ps.active], 0, sizeof(ps.my_cwd[ps.active]));
    ps.active = 0;
    wborder(file_manager[ps.active], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wresize(file_manager[ps.active], dim, COLS);
    wborder(file_manager[ps.active], '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(file_manager[ps.active], 0, 0, "Current:%.*s", COLS / ps.cont - 1 - strlen("Current:"), ps.my_cwd[ps.active]);
}

void scroll_down(char *str)
{
    int real_height = dim - 2;
    ps.current_position[ps.active]++;
    if (ps.current_position[ps.active] == ps.number_of_files[ps.active]) {
        ps.current_position[ps.active]--;
        return;
    }
    if (ps.current_position[ps.active] - real_height == ps.delta[ps.active]) {
        scroll_helper_func(real_height, 1);
        ps.delta[ps.active]++;
        if (!str)
            list_everything(ps.active, ps.current_position[ps.active], 1, 0, 0);
        else {
            mvwprintw(file_manager[ps.active], real_height, 4, str);
            mvwprintw(file_manager[ps.active], INITIAL_POSITION + ps.current_position[ps.active] - ps.delta[ps.active], 1, "->");
        }
    } else {
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active], 1, "  ");
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active] + INITIAL_POSITION, 1, "->");
    }
}

void scroll_up(char *str)
{
    ps.current_position[ps.active]--;
    if (ps.current_position[ps.active] < 0) {
        ps.current_position[ps.active]++;
        return;
    }
    if (ps.current_position[ps.active] < ps.delta[ps.active]) {
        scroll_helper_func(INITIAL_POSITION, - 1);
        ps.delta[ps.active]--;
        if (!str)
            list_everything(ps.active, ps.delta[ps.active], 1, 0, 0);
        else {
            mvwprintw(file_manager[ps.active], INITIAL_POSITION, 4, str);
            mvwprintw(file_manager[ps.active], INITIAL_POSITION + ps.current_position[ps.active] - ps.delta[ps.active], 1, "->");
        }
    } else {
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active] + 2, 1, "  ");
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active] + 1, 1, "->");
    }
}

static void scroll_helper_func(int x, int direction)
{
    mvwprintw(file_manager[ps.active], x, 1, "  ");
    wborder(file_manager[ps.active], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wscrl(file_manager[ps.active], direction);
}

void sync_changes(void)
{
    if (ps.cont == 2) {
        if (strcmp(ps.my_cwd[ps.active], ps.my_cwd[1 - ps.active]) == 0)
            list_everything(1 - ps.active, 0, dim - 2, 1, 1);
    }
}

static void colored_folders(int i, int win)
{
    struct stat file_stat;
    wattron(file_manager[win], A_BOLD);
    if (namelist[win][i]->d_type == DT_DIR)
        wattron(file_manager[win], COLOR_PAIR(1));
    else if (namelist[win][i]->d_type == DT_LNK)
        wattron(file_manager[win], COLOR_PAIR(3));
    else if (namelist[win][i]->d_type == DT_REG) {
        stat(namelist[win][i]->d_name, &file_stat);
        if (file_stat.st_mode & S_IXUSR)
            wattron(file_manager[win], COLOR_PAIR(2));
    }
}

void print_info(char *str, int i)
{
    clear_info(i);
    mvwprintw(info_win, i, 1, str);
    wrefresh(info_win);
}

void clear_info(int i)
{
    wmove(info_win, i, 1);
    wclrtoeol(info_win);
    if ((ps.copied_files) && (ps.pasted == 0))
        mvwprintw(info_win, INFO_LINE, COLS - strlen("File added to copy list."), "File added to copy list.");
    else if (ps.pasted == -1)
        mvwprintw(info_win, INFO_LINE, COLS - strlen("Pasting files..."), "Pasting_files...");
    wrefresh(info_win);
}

void trigger_show_helper_message(void)
{
    int i;
    if (helper_win == NULL) {
        dim = LINES - INFO_HEIGHT - HELPER_HEIGHT;
        for (i = 0; i < ps.cont; i++) {
            wresize(file_manager[i], dim, COLS / ps.cont);
            wborder(file_manager[i], '|', '|', '-', '-', '+', '+', '+', '+');
            mvwprintw(file_manager[i], 0, 0, "Current:%.*s", COLS / ps.cont - 1 - strlen("Current:"), ps.my_cwd[i]);
            if (ps.current_position[i] > dim - 3) {
                ps.current_position[i] = dim - 3 + ps.delta[i];
                mvwprintw(file_manager[i], ps.current_position[i] - ps.delta[i] + INITIAL_POSITION, 1, "->");
            }
            wrefresh(file_manager[i]);
        }
        helper_win = subwin(stdscr, HELPER_HEIGHT, COLS, LINES - INFO_HEIGHT - HELPER_HEIGHT, 0);
        wclear(helper_win);
        helper_print();
        wrefresh(helper_win);
    } else {
        wclear(helper_win);
        delwin(helper_win);
        helper_win = NULL;
        dim = LINES - INFO_HEIGHT;
        for (i = 0; i < ps.cont; i++) {
            wborder(file_manager[i], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
            wresize(file_manager[i], dim, COLS / ps.cont);
            list_everything(i, dim - 2 - HELPER_HEIGHT + ps.delta[i], HELPER_HEIGHT, 0, 0);
        }
    }
}

static void helper_print(void)
{
    wprintw(helper_win, "\n HELPER MESSAGE:\n * n and r to create/remove a file.\n");
    wprintw(helper_win, " * Enter to surf between folders or to open files with $editor var.\n");
    wprintw(helper_win, " * Enter will eventually mount your ISO files in $path.\n");
    wprintw(helper_win, " * You must have fuseiso installed. To unmount, simply press again enter on the same iso file.\n");
    wprintw(helper_win, " * Press h to trigger the showing of hide files. s to see stat about files in current folder.\n");
    wprintw(helper_win, " * c to copy, p to paste, and x to cut a file/dir. p to print a file.\n");
    wprintw(helper_win, " * You can copy as many files/dirs as you want. c again on a file/dir to remove it from copy list.\n");
    wprintw(helper_win, " * o to rename current file/dir; d to create new dir. f to search (case sensitive) for a file.\n");
    wprintw(helper_win, " * t to create new tab (at most one more). w to close tab.\n");
    wprintw(helper_win, " * You can't close first tab. Use q to quit.\n");
    wprintw(helper_win, " * Take a look to /etc/default/ncursesFM.conf file to change some settings.");
    wborder(helper_win, '|', '|', '-', '-', '+', '+', '+', '+');
}

void show_stat(int init, int end, int win)
{
    int i = init;
    unsigned long total_size = 0;
    struct stat file_stat;
    if (init == 0) {
        for (i = 1; i < ps.number_of_files[win]; i++) {
            stat(namelist[win][i]->d_name, &file_stat);
            total_size = total_size + file_stat.st_size;
        }
        mvwprintw(file_manager[win], INITIAL_POSITION, STAT_COL, "Total size: %lu KB", total_size / 1024);
        i = 1;
    }
    for (; ((i < init + end) && (i < ps.number_of_files[win])); i++) {
        if (stat(namelist[win][i]->d_name, &file_stat) == - 1) { //debug
            mvwprintw(file_manager[win], i + INITIAL_POSITION - ps.delta[win], STAT_COL, "%s/%s %s", ps.my_cwd[win], namelist[win][i]->d_name, strerror(errno));
            return;
        }
        mvwprintw(file_manager[win], i + INITIAL_POSITION - ps.delta[win], STAT_COL, "Size: %d KB", file_stat.st_size / 1024);
        wprintw(file_manager[win], "\tPerm: ");
        wprintw(file_manager[win], (S_ISDIR(file_stat.st_mode)) ? "d" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IRUSR) ? "r" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IWUSR) ? "w" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IXUSR) ? "x" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IRGRP) ? "r" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IWGRP) ? "w" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IXGRP) ? "x" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IROTH) ? "r" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IWOTH) ? "w" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IXOTH) ? "x" : "-");
    }
}

void set_nodelay(bool x)
{
    int i;
    for (i = 0; i < ps.cont; i++)
        nodelay(file_manager[i], x);
}
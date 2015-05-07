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
    new_tab();
    info_win = subwin(stdscr, INFO_HEIGHT, COLS, dim, 0);
}

void screen_end(void)
{
    int i;
    for (i = 0; i < cont; i++) {
        wclear(ps[i].file_manager);
        delwin(ps[i].file_manager);
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
    chdir(ps[win].my_cwd);
    if (erase == 1) {
        for (i = 0; i < ps[win].number_of_files; i++)
            free(ps[win].namelist[i]);
        free(ps[win].namelist);
        ps[win].namelist = NULL;
        wclear(ps[win].file_manager);
        if (reset == 1) {
            ps[win].delta = 0;
            ps[win].current_position = 0;
        }
        ps[win].number_of_files = scandir(ps[win].my_cwd, &ps[win].namelist, is_hidden, alphasort);
        my_sort(win);
    }
    wborder(ps[win].file_manager, '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(ps[win].file_manager, 0, 0, "Current:%.*s", COLS / cont - 1 - strlen("Current:"), ps[win].my_cwd);
    for (i = old_dim; (i < ps[win].number_of_files) && (i  < old_dim + end); i++) {
        colored_folders(i, win);
        mvwprintw(ps[win].file_manager, INITIAL_POSITION + i - ps[win].delta, 4, "%.*s", MAX_FILENAME_LENGTH, ps[win].namelist[i]->d_name);
        wattroff(ps[win].file_manager, COLOR_PAIR(1));
        wattroff(ps[win].file_manager, A_BOLD);
    }
    mvwprintw(ps[win].file_manager, INITIAL_POSITION + ps[win].current_position - ps[win].delta, 1, "->");
    if (ps[win].stat_active == 1)
        show_stat(old_dim, end, win);
    wrefresh(ps[win].file_manager);
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
    struct stat file_stat[2];
    int i, j;
    for (i = 0; i < ps[win].number_of_files - 1; i++) {
        lstat(ps[win].namelist[i]->d_name, &file_stat[0]);
        if (!S_ISDIR(file_stat[0].st_mode)) {
            for (j = i + 1; j < ps[win].number_of_files; j++) {
                lstat(ps[win].namelist[j]->d_name, &file_stat[1]);
                if (S_ISDIR(file_stat[1].st_mode)) {
                    temp = ps[win].namelist[j];
                    ps[win].namelist[j] = ps[win].namelist[i];
                    ps[win].namelist[i] = temp;
                    break;
                }
            }
        }
    }
}

void new_tab(void)
{
    cont++;
    if (cont == 2) {
        wresize(ps[active].file_manager, dim, COLS / cont);
        wborder(ps[active].file_manager, '|', '|', '-', '-', '+', '+', '+', '+');
        mvwprintw(ps[active].file_manager, 0, 0, "Current:%.*s", COLS / cont - 1 - strlen("Current:"), ps[active].my_cwd);
        wrefresh(ps[active].file_manager);
    }
    active = cont - 1;
    ps[active].file_manager = subwin(stdscr, dim, COLS / cont, 0, (COLS * active) / cont);
    keypad(ps[active].file_manager, TRUE);
    scrollok(ps[active].file_manager, TRUE);
    idlok(ps[active].file_manager, TRUE);
    if (config.starting_dir) {
        if ((cont == 1) || (config.second_tab_starting_dir != 0))
            strcpy(ps[active].my_cwd, config.starting_dir);
    }
    if (strlen(ps[active].my_cwd) == 0)
        getcwd(ps[active].my_cwd, PATH_MAX);
    list_everything(active, 0, dim - 2, 1, 1);
}

void delete_tab(void)
{
    int i;
    cont--;
    wclear(ps[active].file_manager);
    delwin(ps[active].file_manager);
    ps[active].file_manager = NULL;
    for (i = ps[active].number_of_files - 1; i >= 0; i--)
        free(ps[active].namelist[i]);
    free(ps[active].namelist);
    ps[active].namelist = NULL;
    ps[active].number_of_files = 0;
    ps[active].stat_active = 0;
    memset(ps[active].my_cwd, 0, sizeof(ps[active].my_cwd));
    active = cont - 1;
    wborder(ps[active].file_manager, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wresize(ps[active].file_manager, dim, COLS);
    wborder(ps[active].file_manager, '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(ps[active].file_manager, 0, 0, "Current:%.*s", COLS / cont - 1 - strlen("Current:"), ps[active].my_cwd);
}

void scroll_down(char *str)
{
    int real_height = dim - 2;
    if (ps[active].current_position < ps[active].number_of_files - 1) {
        ps[active].current_position++;
        if (ps[active].current_position - real_height == ps[active].delta) {
            scroll_helper_func(real_height, 1);
            ps[active].delta++;
            if (!str)
                list_everything(active, ps[active].current_position, 1, 0, 0);
            else {
                mvwprintw(ps[active].file_manager, real_height, 4, str);
                mvwprintw(ps[active].file_manager, INITIAL_POSITION + ps[active].current_position - ps[active].delta, 1, "->");
            }
        } else {
            mvwprintw(ps[active].file_manager, ps[active].current_position - ps[active].delta, 1, "  ");
            mvwprintw(ps[active].file_manager, ps[active].current_position - ps[active].delta + INITIAL_POSITION, 1, "->");
        }
    }
}

void scroll_up(char *str)
{
    if (ps[active].current_position > 0) {
        ps[active].current_position--;
        if (ps[active].current_position < ps[active].delta) {
            scroll_helper_func(INITIAL_POSITION, - 1);
            ps[active].delta--;
            if (!str)
                list_everything(active, ps[active].delta, 1, 0, 0);
            else {
                mvwprintw(ps[active].file_manager, INITIAL_POSITION, 4, str);
                mvwprintw(ps[active].file_manager, INITIAL_POSITION + ps[active].current_position - ps[active].delta, 1, "->");
            }
        } else {
            mvwprintw(ps[active].file_manager, ps[active].current_position - ps[active].delta + 2, 1, "  ");
            mvwprintw(ps[active].file_manager, ps[active].current_position - ps[active].delta + 1, 1, "->");
        }
    }
}

static void scroll_helper_func(int x, int direction)
{
    mvwprintw(ps[active].file_manager, x, 1, "  ");
    wborder(ps[active].file_manager, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wscrl(ps[active].file_manager, direction);
}

void sync_changes(void)
{
    if (cont == 2) {
        if (strcmp(ps[active].my_cwd, ps[1 - active].my_cwd) == 0)
            list_everything(1 - active, 0, dim - 2, 1, 1);
    }
    list_everything(active, 0, dim - 2, 1, 1);
}

static void colored_folders(int i, int win)
{
    struct stat file_stat;
    lstat(ps[win].namelist[i]->d_name, &file_stat);
    wattron(ps[win].file_manager, A_BOLD);
    if (S_ISDIR(file_stat.st_mode))
        wattron(ps[win].file_manager, COLOR_PAIR(1));
    else if (S_ISLNK(file_stat.st_mode))
        wattron(ps[win].file_manager, COLOR_PAIR(3));
    else if ((S_ISREG(file_stat.st_mode)) && (file_stat.st_mode & S_IXUSR))
        wattron(ps[win].file_manager, COLOR_PAIR(2));
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
    if ((copied_files) && (pasted == 0))
        mvwprintw(info_win, INFO_LINE, COLS - strlen("File added to copy list."), "File added to copy list.");
    else if (pasted == -1)
        mvwprintw(info_win, INFO_LINE, COLS - strlen("Pasting files..."), "Pasting_files...");
    wrefresh(info_win);
}

void trigger_show_helper_message(void)
{
    int i;
    if (helper_win == NULL) {
        dim = LINES - INFO_HEIGHT - HELPER_HEIGHT;
        for (i = 0; i < cont; i++) {
            wresize(ps[i].file_manager, dim, COLS / cont);
            wborder(ps[i].file_manager, '|', '|', '-', '-', '+', '+', '+', '+');
            mvwprintw(ps[i].file_manager, 0, 0, "Current:%.*s", COLS / cont - 1 - strlen("Current:"), ps[i].my_cwd);
            if (ps[i].current_position > dim - 3) {
                ps[i].current_position = dim - 3 + ps[i].delta;
                mvwprintw(ps[i].file_manager, ps[i].current_position - ps[i].delta + INITIAL_POSITION, 1, "->");
            }
            wrefresh(ps[i].file_manager);
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
        for (i = 0; i < cont; i++) {
            wborder(ps[i].file_manager, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
            wresize(ps[i].file_manager, dim, COLS / cont);
            list_everything(i, dim - 2 - HELPER_HEIGHT + ps[i].delta, HELPER_HEIGHT, 0, 0);
        }
    }
}

static void helper_print(void)
{
    wprintw(helper_win, "\n HELPER MESSAGE:\n * n and r to create/remove a file.\n");
    wprintw(helper_win, " * Enter to surf between folders or to open files with $editor var.\n");
    wprintw(helper_win, " * Enter will eventually mount your ISO/compressed files.\n");
    wprintw(helper_win, " * You must have archivemount installed. To unmount, simply press again enter on the same iso file.\n");
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
        for (i = 1; i < ps[win].number_of_files; i++) {
            stat(ps[win].namelist[i]->d_name, &file_stat);
            total_size = total_size + file_stat.st_size;
        }
        mvwprintw(ps[win].file_manager, INITIAL_POSITION, STAT_COL, "Total size: %lu KB", total_size / 1024);
        i = 1;
    }
    for (; ((i < init + end) && (i < ps[win].number_of_files)); i++) {
        stat(ps[win].namelist[i]->d_name, &file_stat);
        mvwprintw(ps[win].file_manager, i + INITIAL_POSITION - ps[win].delta, STAT_COL, "Size: %d KB", file_stat.st_size / 1024);
        wprintw(ps[win].file_manager, "\tPerm: ");
        wprintw(ps[win].file_manager, (S_ISDIR(file_stat.st_mode)) ? "d" : "-");
        wprintw(ps[win].file_manager, (file_stat.st_mode & S_IRUSR) ? "r" : "-");
        wprintw(ps[win].file_manager, (file_stat.st_mode & S_IWUSR) ? "w" : "-");
        wprintw(ps[win].file_manager, (file_stat.st_mode & S_IXUSR) ? "x" : "-");
        wprintw(ps[win].file_manager, (file_stat.st_mode & S_IRGRP) ? "r" : "-");
        wprintw(ps[win].file_manager, (file_stat.st_mode & S_IWGRP) ? "w" : "-");
        wprintw(ps[win].file_manager, (file_stat.st_mode & S_IXGRP) ? "x" : "-");
        wprintw(ps[win].file_manager, (file_stat.st_mode & S_IROTH) ? "r" : "-");
        wprintw(ps[win].file_manager, (file_stat.st_mode & S_IWOTH) ? "w" : "-");
        wprintw(ps[win].file_manager, (file_stat.st_mode & S_IXOTH) ? "x" : "-");
    }
}

void set_nodelay(bool x)
{
    int i;
    for (i = 0; i < cont; i++)
        nodelay(ps[i].file_manager, x);
}
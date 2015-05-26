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
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    raw();
    noecho();
    curs_set(0);
    dim = LINES - INFO_HEIGHT;
    new_tab();
    info_win = subwin(stdscr, INFO_HEIGHT, COLS, dim, 0);
    mvwprintw(info_win, INFO_LINE, 1, "INFO: ");
    mvwprintw(info_win, ERR_LINE, 1, "ERR: ");
    wrefresh(info_win);
}

void screen_end(void)
{
    int i;
    for (i = 0; i < cont; i++) {
        wclear(ps[i].fm);
        delwin(ps[i].fm);
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

void generate_list(int win)
{
    int i;
    struct dirent **files;
    chdir(ps[win].my_cwd);
    for (i = 0; i < ps[win].number_of_files; i++)
        free(ps[win].nl[i]);
    free(ps[win].nl);
    ps[win].nl = NULL;
    ps[win].number_of_files = scandir(ps[win].my_cwd, &files, is_hidden, alphasort);
    ps[win].nl = malloc(sizeof(char *) * ps[win].number_of_files);
    for (i = 0; i < ps[win].number_of_files; i++) {
        ps[win].nl[i] = malloc(sizeof(char) * PATH_MAX);
        strcpy(ps[win].nl[i], files[i]->d_name);
    }
    my_sort(win);
    for (i = ps[win].number_of_files - 1; i >= 0; i--)
        free(files[i]);
    free(files);
    wclear(ps[win].fm);
    ps[win].delta = 0;
    ps[win].curr_pos = 0;
    list_everything(win, 0, dim - 2, ps[win].nl);
    chdir(ps[active].my_cwd);
}

void list_everything(int win, int old_dim, int end, char **files)
{
    int i, max_length;
    const char *search_mess = "q to leave search win";
    wborder(ps[win].fm, '|', '|', '-', '-', '+', '+', '+', '+');
    if (!search_mode) {
        mvwprintw(ps[win].fm, 0, 0, "Current:%.*s", width[win] - 1 - strlen("Current:"), ps[win].my_cwd);
        max_length = MAX_FILENAME_LENGTH;
    } else {
        mvwprintw(ps[win].fm, 0, 0, "Found file searching %.*s: ", width[active] - 1 - strlen("Found file searching : ") - strlen(search_mess), searched_string);
        mvwprintw(ps[win].fm, 0, width[win] - strlen(search_mess), search_mess);
        max_length = width[win] - 5;
    }
    wattron(ps[win].fm, A_BOLD);
    for (i = old_dim; (i < ps[win].number_of_files) && (i  < old_dim + end); i++) {
        colored_folders(win, files[i]);
        mvwprintw(ps[win].fm, INITIAL_POSITION + i - ps[win].delta, 4, "%.*s", max_length, files[i]);
        wattroff(ps[win].fm, COLOR_PAIR);
    }
    wattroff(ps[win].fm, A_BOLD);
    mvwprintw(ps[win].fm, INITIAL_POSITION + ps[win].curr_pos - ps[win].delta, 1, "->");
    if ((!search_mode) && (ps[win].stat_active == 1))
        show_stat(old_dim, end, win);
    wrefresh(ps[win].fm);
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
    char temp[PATH_MAX];
    struct stat file_stat[2];
    int i, j;
    for (i = 0; i < ps[win].number_of_files - 1; i++) {
        lstat(ps[win].nl[i], &file_stat[0]);
        if (!S_ISDIR(file_stat[0].st_mode)) {
            for (j = i + 1; j < ps[win].number_of_files; j++) {
                lstat(ps[win].nl[j], &file_stat[1]);
                if (S_ISDIR(file_stat[1].st_mode)) {
                    strcpy(temp, ps[win].nl[j]);
                    strcpy(ps[win].nl[j], ps[win].nl[i]);
                    strcpy(ps[win].nl[i], temp);
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
        width[active] = COLS / cont;
        wresize(ps[active].fm, dim, width[active]);
        wborder(ps[active].fm, '|', '|', '-', '-', '+', '+', '+', '+');
        mvwprintw(ps[active].fm, 0, 0, "Current:%.*s", width[active] - 1 - strlen("Current:"), ps[active].my_cwd);
        wrefresh(ps[active].fm);
    }
    active = cont - 1;
    width[active] = COLS / cont + COLS % cont;
    ps[active].fm = subwin(stdscr, dim, width[active], 0, (COLS * active) / cont);
    keypad(ps[active].fm, TRUE);
    scrollok(ps[active].fm, TRUE);
    idlok(ps[active].fm, TRUE);
    if (config.starting_dir) {
        if ((cont == 1) || (config.second_tab_starting_dir != 0))
            strcpy(ps[active].my_cwd, config.starting_dir);
    }
    if (strlen(ps[active].my_cwd) == 0)
        getcwd(ps[active].my_cwd, PATH_MAX);
    chdir(ps[active].my_cwd);
    generate_list(active);
}

void delete_tab(void)
{
    int i;
    cont--;
    wclear(ps[active].fm);
    delwin(ps[active].fm);
    ps[active].fm = NULL;
    for (i = ps[active].number_of_files - 1; i >= 0; i--)
        free(ps[active].nl[i]);
    free(ps[active].nl);
    ps[active].nl = NULL;
    ps[active].number_of_files = 0;
    ps[active].stat_active = 0;
    memset(ps[active].my_cwd, 0, sizeof(ps[active].my_cwd));
    active = cont - 1;
    width[active] = COLS;
    wborder(ps[active].fm, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wresize(ps[active].fm, dim, width[active]);
    wborder(ps[active].fm, '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(ps[active].fm, 0, 0, "Current:%.*s", width[active] - 1 - strlen("Current:"), ps[active].my_cwd);
}

void scroll_down(char **str)
{
    int real_height = dim - 2;
    if (ps[active].curr_pos < ps[active].number_of_files - 1) {
        ps[active].curr_pos++;
        if (ps[active].curr_pos - real_height == ps[active].delta) {
            scroll_helper_func(real_height, 1);
            ps[active].delta++;
            list_everything(active, ps[active].curr_pos, 1, str);
        } else {
            mvwprintw(ps[active].fm, ps[active].curr_pos - ps[active].delta, 1, "  ");
            mvwprintw(ps[active].fm, ps[active].curr_pos - ps[active].delta + INITIAL_POSITION, 1, "->");
        }
    }
}

void scroll_up(char **str)
{
    if (ps[active].curr_pos > 0) {
        ps[active].curr_pos--;
        if (ps[active].curr_pos < ps[active].delta) {
            scroll_helper_func(INITIAL_POSITION, - 1);
            ps[active].delta--;
            list_everything(active, ps[active].delta, 1, str);
        } else {
            mvwprintw(ps[active].fm, ps[active].curr_pos - ps[active].delta + 2, 1, "  ");
            mvwprintw(ps[active].fm, ps[active].curr_pos - ps[active].delta + 1, 1, "->");
        }
    }
}

static void scroll_helper_func(int x, int direction)
{
    mvwprintw(ps[active].fm, x, 1, "  ");
    wborder(ps[active].fm, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wscrl(ps[active].fm, direction);
}

void sync_changes(void)
{
    if (cont == 2) {
        if (strcmp(ps[active].my_cwd, ps[1 - active].my_cwd) == 0) {
            generate_list(1 - active);
        }
    }
    generate_list(active);
}

static void colored_folders(int win, char *name)
{
    struct stat file_stat;
    if (lstat(name, &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode))
            wattron(ps[win].fm, COLOR_PAIR(1));
        else if (S_ISLNK(file_stat.st_mode))
            wattron(ps[win].fm, COLOR_PAIR(3));
        else if ((S_ISREG(file_stat.st_mode)) && (file_stat.st_mode & S_IXUSR))
            wattron(ps[win].fm, COLOR_PAIR(2));
    } else {
        wattron(ps[win].fm, COLOR_PAIR(4));
    }
}

void print_info(const char *str, int i)
{
    const char *extracting_mess = "Extracting...";
    const char *searching_mess = "Searching...";
    const char *found_searched_mess = "Search finished. Press f anytime to view the results.";
    int mess_line = INFO_LINE, j;
    for (j = INFO_LINE; j < 2; j++) {
        wmove(info_win, j, strlen("INFO: ") + 1);
        wclrtoeol(info_win);
    }
    if (strlen(info_message)) {
        mvwprintw(info_win, INFO_LINE, COLS - strlen(info_message), info_message);
        mess_line = ERR_LINE;
    }
    if (extracting == 1)
        mvwprintw(info_win, mess_line, COLS - strlen(extracting_mess), extracting_mess);
    if (searching == 1)
        mvwprintw(info_win, mess_line, COLS - strlen(searching_mess), searching_mess);
    if ((searching == 2) && (!search_mode))
        mvwprintw(info_win, mess_line, COLS - strlen(found_searched_mess), found_searched_mess);
    if (str) {
        if (i == INFO_LINE)
            mvwprintw(info_win, i, strlen("INFO: ") + 1, str);
        else
            mvwprintw(info_win, i, strlen("ERR: ") + 1, str);
    }
    wrefresh(info_win);
}

void trigger_show_helper_message(void)
{
    int i;
    if (helper_win == NULL) {
        dim = LINES - INFO_HEIGHT - HELPER_HEIGHT;
        for (i = 0; i < cont; i++) {
            wresize(ps[i].fm, dim, width[i]);
            wborder(ps[i].fm, '|', '|', '-', '-', '+', '+', '+', '+');
            mvwprintw(ps[i].fm, 0, 0, "Current:%.*s", width[i] - 1 - strlen("Current:"), ps[i].my_cwd);
            if (ps[i].curr_pos > dim - 3) {
                ps[i].curr_pos = dim - 3 + ps[i].delta;
                mvwprintw(ps[i].fm, ps[i].curr_pos - ps[i].delta + INITIAL_POSITION, 1, "->");
            }
            wrefresh(ps[i].fm);
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
            wborder(ps[i].fm, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
            wresize(ps[i].fm, dim, width[i]);
            list_everything(i, dim - 2 - HELPER_HEIGHT + ps[i].delta, HELPER_HEIGHT, ps[i].nl);
        }
    }
}

static void helper_print(void)
{
    wprintw(helper_win, "\n HELPER MESSAGE:\n * n and r to create/remove a file.\n");
    wprintw(helper_win, " * Enter to surf between folders or to open files with $editor var.\n");
    wprintw(helper_win, " * Enter will eventually ask to extract archives, or mount your ISO files.\n");
    wprintw(helper_win, " * You must have isomount installed. To unmount, simply press again enter on the same iso file.\n");
    wprintw(helper_win, " * Press h to trigger the showing of hide files. s to see stat about files in current folder.\n");
    wprintw(helper_win, " * c or x to select files. v to paste: files will be copied if selected with c, or cut if selected with x.\n");
    wprintw(helper_win, " * p to print a file. b to compress selected files.\n");
    wprintw(helper_win, " * You can copy as many files/dirs as you want. c again on a file/dir to remove it from file list.\n");
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
            stat(ps[win].nl[i], &file_stat);
            total_size = total_size + file_stat.st_size;
        }
        mvwprintw(ps[win].fm, INITIAL_POSITION, STAT_COL, "Total size: %lu KB", total_size / 1024);
        i = 1;
    }
    for (; ((i < init + end) && (i < ps[win].number_of_files)); i++) {
        stat(ps[win].nl[i], &file_stat);
        mvwprintw(ps[win].fm, i + INITIAL_POSITION - ps[win].delta, STAT_COL, "Size: %d KB", file_stat.st_size / 1024);
        wprintw(ps[win].fm, "\tPerm: ");
        wprintw(ps[win].fm, (S_ISDIR(file_stat.st_mode)) ? "d" : "-");
        wprintw(ps[win].fm, (file_stat.st_mode & S_IRUSR) ? "r" : "-");
        wprintw(ps[win].fm, (file_stat.st_mode & S_IWUSR) ? "w" : "-");
        wprintw(ps[win].fm, (file_stat.st_mode & S_IXUSR) ? "x" : "-");
        wprintw(ps[win].fm, (file_stat.st_mode & S_IRGRP) ? "r" : "-");
        wprintw(ps[win].fm, (file_stat.st_mode & S_IWGRP) ? "w" : "-");
        wprintw(ps[win].fm, (file_stat.st_mode & S_IXGRP) ? "x" : "-");
        wprintw(ps[win].fm, (file_stat.st_mode & S_IROTH) ? "r" : "-");
        wprintw(ps[win].fm, (file_stat.st_mode & S_IWOTH) ? "w" : "-");
        wprintw(ps[win].fm, (file_stat.st_mode & S_IXOTH) ? "x" : "-");
    }
}

void erase_stat(void)
{
    int i;
    for (i = 0; i < ps[active].number_of_files && i < dim - 2; i++) {
        wmove(ps[active].fm, i + 1, STAT_COL);
        wclrtoeol(ps[active].fm);
    }
    wborder(ps[active].fm, '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(ps[active].fm, 0, 0, "Current:%.*s", width[active] - 1 - strlen("Current:"), ps[active].my_cwd);
}

void set_nodelay(bool x)
{
    int i;
    for (i = 0; i < cont; i++)
        nodelay(ps[i].fm, x);
}
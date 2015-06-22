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

static int is_hidden(const struct dirent *current_file);
static void scroll_helper_func(int x, int direction);
static void colored_folders(int win, const char *name);
static void helper_print(void);
static void change_unit(float size, char *str);

WINDOW *helper_win = NULL;
static int dim, width[MAX_TABS];

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
    mvwprintw(info_win, INFO_LINE, 1, "I: ");
    mvwprintw(info_win, ERR_LINE, 1, "E: ");
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
    int i, number_of_files;
    struct dirent **files;

    if (sv.searching == 3 + win) {
        return;
    }
    free_str(ps[win].nl);
    number_of_files = scandir(ps[win].my_cwd, &files, is_hidden, alphasort);
    for (i = 0; i < number_of_files; i++) {
        if (!(ps[win].nl[i] = safe_malloc(sizeof(char) * PATH_MAX, "No more memory available. Program will exit."))) {
            quit_thread_func(th);
            quit_thread_func(extractor_th);
            free_everything();
            screen_end();
            exit(1);
        }
        sprintf(ps[win].nl[i], "%s/%s", ps[win].my_cwd, files[i]->d_name);
    }
    for (i = number_of_files - 1; i >= 0; i--) {
        free(files[i]);
    }
    free(files);
    wclear(ps[win].fm);
    ps[win].delta = 0;
    ps[win].curr_pos = 0;
    list_everything(win, 0, dim - 2, ps[win].nl);
}

void list_everything(int win, int old_dim, int end, char **files)
{
    int i;
    const char *search_mess = "q to leave search win";

    wborder(ps[win].fm, '|', '|', '-', '-', '+', '+', '+', '+');
    if (sv.searching != 3 + win) {
        mvwprintw(ps[win].fm, 0, 0, "Current:%.*s", width[win] - 1 - strlen("Current:"), ps[win].my_cwd);
    } else {
        mvwprintw(ps[win].fm, 0, 0, "Found file searching %.*s: ", width[active] - 1 - strlen("Found file searching : ") - strlen(search_mess), sv.searched_string);
        mvwprintw(ps[win].fm, 0, width[win] - (strlen(search_mess) + 1), search_mess);
    }
    if (end == 0) {
        end = dim - 2;
    }
    wattron(ps[win].fm, A_BOLD);
    for (i = old_dim; (files[i]) && (i  < old_dim + end); i++) {
        colored_folders(win, files[i]);
        if (sv.searching == 3 + win) {
            mvwprintw(ps[win].fm, INITIAL_POSITION + i - ps[win].delta, 4, "%.*s", width[win] - 5, files[i]);
        } else {
            mvwprintw(ps[win].fm, INITIAL_POSITION + i - ps[win].delta, 4, "%.*s", MAX_FILENAME_LENGTH, strrchr(files[i], '/') + 1);
        }
        wattroff(ps[win].fm, COLOR_PAIR);
    }
    wattroff(ps[win].fm, A_BOLD);
    mvwprintw(ps[win].fm, INITIAL_POSITION + ps[win].curr_pos - ps[win].delta, 1, "->");
    if ((sv.searching != 3 + win) && (ps[win].stat_active == 1)) {
        show_stat(old_dim, end, win);
    }
    wrefresh(ps[win].fm);
}

static int is_hidden(const struct dirent *current_file)
{
    if ((strlen(current_file->d_name) == 1) && (current_file->d_name[0] == '.')) {
        return (FALSE);
    }
    if (config.show_hidden == 0) {
        if (strlen(current_file->d_name) > 1 && current_file->d_name[0] == '.' && current_file->d_name[1] != '.') {
            return (FALSE);
        }
        return (TRUE);
    }
    return (TRUE);
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
        if ((cont == 1) || (config.second_tab_starting_dir != 0)){
            strcpy(ps[active].my_cwd, config.starting_dir);
        }
    }
    if (strlen(ps[active].my_cwd) == 0) {
        getcwd(ps[active].my_cwd, PATH_MAX);
    }
    chdir(ps[active].my_cwd);
    generate_list(active);
}

void delete_tab(void)
{
    cont--;
    wclear(ps[active].fm);
    delwin(ps[active].fm);
    ps[active].fm = NULL;
    free_str(ps[active].nl);
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
    if (str[ps[active].curr_pos + 1]) {
        ps[active].curr_pos++;
        if (ps[active].curr_pos - (dim - 2) == ps[active].delta) {
            scroll_helper_func(dim - 2, 1);
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
            scroll_helper_func(INITIAL_POSITION, -1);
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
    ps[active].delta += direction;
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

static void colored_folders(int win, const char *name)
{
    struct stat file_stat;

    if (lstat(name, &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode)) {
            wattron(ps[win].fm, COLOR_PAIR(1));
        } else if (S_ISLNK(file_stat.st_mode)) {
            wattron(ps[win].fm, COLOR_PAIR(3));
        } else if ((S_ISREG(file_stat.st_mode)) && (file_stat.st_mode & S_IXUSR)) {
            wattron(ps[win].fm, COLOR_PAIR(2));
        }
    } else {
        wattron(ps[win].fm, COLOR_PAIR(4));
    }
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
        scrollok(helper_win, TRUE);
        idlok(helper_win, TRUE);
        helper_print();
    } else {
        for (i = 0; i <= HELPER_HEIGHT - 2; i++) {
            wscrl(helper_win, -1);
            mvwhline(helper_win, HELPER_HEIGHT - 1, 1, '-', COLS - 2);
//             mvwaddch(helper_win, HELPER_HEIGHT - 1, COLS - 1, 'X'); // -> ncurses bug?
//             mvwaddch(helper_win, HELPER_HEIGHT - 1, 0, '+');
            wrefresh(helper_win);
            usleep(30000);
        }
        delwin(helper_win);
        helper_win = NULL;
        dim = LINES - INFO_HEIGHT;
        for (i = 0; i < cont; i++) {
            mvwhline(ps[i].fm, dim - HELPER_HEIGHT - 1, 0, ' ', COLS - 1);
            wresize(ps[i].fm, dim, width[i]);
            list_everything(i, dim - 2 - HELPER_HEIGHT + ps[i].delta, HELPER_HEIGHT, ps[i].nl);
        }
    }
}

static void helper_print(void)
{
    int i;
    const char *helper_string[HELPER_HEIGHT - 2] = { "n and r to create/remove a file.",
        "Enter to surf between folders or to open files with either xdg-open (if in a X session) or (text only) $editor var.",
        "Enter will eventually ask to extract archives, or mount your ISO files.",
        "To mount ISO you must have isomount installed. To unmount, simply press again enter on the same iso file.",
        "Press h to trigger the showing of hide files. s to see stat about files in current folder.",
        "c or x to select files. v to paste: files will be copied if selected with c, or cut if selected with x.",
        "p to print a file. b to compress selected files. a to view md5/shasum of highlighted file.",
        "You can copy as many files/dirs as you want. c/x again on a file/dir to remove it from file list.",
        "o to rename current file/dir; d to create new dir. f to search (case sensitive) for a file.",
        "t to create new tab (at most one more). w to close tab. u to view current file's mimetype.",
        "You can't close first tab. Use q to quit.",
        "Take a look to /etc/default/ncursesFM.conf file to change some settings."};

    mvwhline(helper_win, HELPER_HEIGHT - 1, 1, ' ', COLS - 2);
    for (i = 0; i < HELPER_HEIGHT - 2; i++) {
        mvwprintw(helper_win, HELPER_HEIGHT - 1, 0, " * %s", helper_string[i]);
        wscrl(helper_win, 1);
        wrefresh(helper_win);
        usleep(30000);
    }
    wborder(helper_win, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(helper_win);
}

void show_stat(int init, int end, int win)
{
    int i = init, j;
    int perm_bit[9] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    char perm_sign[3] = {'r', 'w', 'x'}, str[20];
    float total_size = 0;
    struct stat file_stat;

    if (init == 0) {
        for (i = 1; ps[win].nl[i]; i++) {
            stat(ps[win].nl[i], &file_stat);
            total_size += file_stat.st_size;
        }
        change_unit(total_size, str);
        mvwprintw(ps[win].fm, INITIAL_POSITION, STAT_COL, "Total size: %s", str);
        i = 1;
    }
    for (; ((i < init + end) && (ps[win].nl[i])); i++) {
        stat(ps[win].nl[i], &file_stat);
        change_unit(file_stat.st_size, str);
        mvwprintw(ps[win].fm, i + INITIAL_POSITION - ps[win].delta, STAT_COL, "%s", str);
        wprintw(ps[win].fm, "\t");
        for (j = 0; j < 9; j++) {
            wprintw(ps[win].fm, (file_stat.st_mode & perm_bit[j]) ? "%c" : "-", perm_sign[j % 3]);
        }
    }
}

static void change_unit(float size, char *str)
{
    char *unit[3] = {"KB", "MB", "GB"};
    int i = 0;

    size /= 1024;
    while ((size > 1024) && (i < 3)) {
        size /= 1024;
        i++;
    }
    sprintf(str, "%.2f%s", size, unit[i]);
}

void erase_stat(void)
{
    int i;

    for (i = 0; (ps[active].nl[i]) && (i < dim - 2); i++) {
        wmove(ps[active].fm, i + 1, STAT_COL);
        wclrtoeol(ps[active].fm);
    }
    wborder(ps[active].fm, '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(ps[active].fm, 0, 0, "Current:%.*s", width[active] - 1 - strlen("Current:"), ps[active].my_cwd);
}
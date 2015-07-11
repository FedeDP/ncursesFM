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

static void print_border_and_title(int win);
static int is_hidden(const struct dirent *current_file);
static void scroll_helper_func(int x, int direction);
static void colored_folders(int win, const char *name);
static void helper_print(void);
static void create_helper_win(void);
static void remove_helper_win(void);
static void change_unit(float size, char *str);

static WINDOW *helper_win = NULL, *info_win = NULL;
static int dim, width[MAX_TABS];

/*
 * Initializes screen, create first tab, and create info_win
 */
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
    while (cont < config.starting_tabs) {
        new_tab();
    }
    info_win = subwin(stdscr, INFO_HEIGHT, COLS, dim, 0);
    mvwprintw(info_win, INFO_LINE, 1, "I: ");
    mvwprintw(info_win, ERR_LINE, 1, "E: ");
    wrefresh(info_win);
}

/*
 * Clear any existing window, and remove stdscr
 */
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

/*
 * Creates a list of strings from current win path's files.
 * It won't do anything if window 'win' is in search mode.
 * If program cannot allocate memory, it will leave.
 */
void generate_list(int win)
{
    int i, number_of_files;
    struct dirent **files;

    if (sv.searching == 3 + win) {
        return;
    }
    pthread_mutex_lock(&lock);
    free_str(ps[win].nl);
    number_of_files = scandir(ps[win].my_cwd, &files, is_hidden, alphasort);
    for (i = 0; i < number_of_files; i++) {
        if (!(ps[win].nl[i] = safe_malloc(sizeof(char) * PATH_MAX, fatal_mem_error))) {
            quit = 1;
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
    pthread_mutex_unlock(&lock);
}

/*
 * Prints to window 'win' a list of strings.
 * Check if window 'win' is in search mode, and takes care.
 * If end == 0, it means it needs to print every string until the end of available rows,
 * else it indicates how many strings the function has to print (starting from files[old_dim])
 * If stat_active == 1 for 'win', and 'win' is not in search mode, it prints stats about size and permissions for every file.
 */
void list_everything(int win, int old_dim, int end, char *files[])
{
    int i;

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
    print_border_and_title(win);
}

/*
 * Helper function that prints borders and title of 'win'
 */
static void print_border_and_title(int win)
{
    wborder(ps[win].fm, '|', '|', '-', '-', '+', '+', '+', '+');
    if (sv.searching != 3 + win) {
        mvwprintw(ps[win].fm, 0, 0, "Current:%.*s", width[win] - 1 - strlen("Current:"), ps[win].my_cwd);
    } else {
        mvwprintw(ps[win].fm, 0, 0, "Found file searching %.*s: ", width[active] - 1 - strlen("Found file searching : ") - strlen(search_tab_title), sv.searched_string);
        mvwprintw(ps[win].fm, 0, width[win] - (strlen(search_tab_title) + 1), search_tab_title);
    }
    wrefresh(ps[win].fm);
}

/*
 * Helper function passed to scandir (in generate_list() )
 * Will return false for '.', and for every file starting with '.' (except for '..') if !show_hidden
 */
static int is_hidden(const struct dirent *current_file)
{
    if (current_file->d_name[0] == '.') {
        if ((strlen(current_file->d_name) == 1) || ((!config.show_hidden) && current_file->d_name[1] != '.')) {
            return (FALSE);
        }
    }
    return (TRUE);
}

/*
 * Creates a new tab and gives new tab the active flag.
 * Then calculates new tab cwd, chdir there, and generate its list of files.
 */
void new_tab(void)
{
    if (cont == MAX_TABS) {
        return;
    }
    cont++;
    if (cont == 2) {
        width[active] = COLS / cont;
        wresize(ps[active].fm, dim, width[active]);
        print_border_and_title(active);
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
    wclear(ps[!active].fm);
    delwin(ps[!active].fm);
    ps[!active].fm = NULL;
    free_str(ps[!active].nl);
    ps[!active].stat_active = 0;
    memset(ps[!active].my_cwd, 0, sizeof(ps[!active].my_cwd));
    width[active] = COLS;
    wborder(ps[active].fm, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wresize(ps[active].fm, dim, width[active]);
    print_border_and_title(active);
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

/*
 * Check which of the "cont" tabs is in the "str" path,
 * then refresh it.
 */
void sync_changes(const char *str)
{
    int i;

    for (i = 0; i < cont; i++) {
        if (strcmp(str, ps[i].my_cwd) == 0) {
            generate_list(i);
        }
    }
}

/*
 * Follow ls color scheme to color files/folders. Archives are yellow.
 * Receives a win and a full path to be checked.
 */
static void colored_folders(int win, const char *name)
{
    struct stat file_stat;

    if (lstat(name, &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode)) {
            wattron(ps[win].fm, COLOR_PAIR(1));
        } else if (S_ISLNK(file_stat.st_mode)) {
            wattron(ps[win].fm, COLOR_PAIR(3));
        } else if (is_archive(name)) {
            wattron(ps[win].fm, COLOR_PAIR(4));
        } else if ((S_ISREG(file_stat.st_mode)) && (file_stat.st_mode & S_IXUSR)) {
            wattron(ps[win].fm, COLOR_PAIR(2));
        }
    } else {
        wattron(ps[win].fm, COLOR_PAIR(4));
    }
}

void trigger_show_helper_message(void)
{
    if (helper_win == NULL) {
        create_helper_win();
    } else {
        remove_helper_win();
    }
}

/*
 * Changes "dim" global var;
 * if current position in the folder was > dim - 3 (where dim goes from 0 to dim - 1, and -2 is because of helper_win borders),
 * change it to dim - 3 + ps[i].delta.
 * Then create helper_win and print its strings.
 */
static void create_helper_win(void)
{
    int i;

    dim = LINES - INFO_HEIGHT - HELPER_HEIGHT;
    for (i = 0; i < cont; i++) {
        wresize(ps[i].fm, dim, width[i]);
        print_border_and_title(i);
        if (ps[i].curr_pos > dim - 3) {
            ps[i].curr_pos = dim - 3 + ps[i].delta;
            mvwprintw(ps[i].fm, dim - 3 + INITIAL_POSITION, 1, "->");
        }
        wrefresh(ps[i].fm);
    }
    helper_win = subwin(stdscr, HELPER_HEIGHT, COLS, LINES - INFO_HEIGHT - HELPER_HEIGHT, 0);
    wclear(helper_win);
    helper_print();
}

static void remove_helper_win(void)
{
    int i;

    wclear(helper_win);
    delwin(helper_win);
    helper_win = NULL;
    dim = LINES - INFO_HEIGHT;
    for (i = 0; i < cont; i++) {
        mvwhline(ps[i].fm, dim - 1 - HELPER_HEIGHT, 0, ' ', COLS);
        wresize(ps[i].fm, dim, width[i]);
        list_everything(i, dim - 2 - HELPER_HEIGHT + ps[i].delta, HELPER_HEIGHT, ps[i].nl);
    }
}

static void helper_print(void)
{
    int i;

    wborder(helper_win, '|', '|', '-', '-', '+', '+', '+', '+');
    for (i = HELPER_HEIGHT - INFO_HEIGHT - 1; i >= 0; i--) {
        mvwprintw(helper_win, i + 1, 0, "| * %s", helper_string[i]);
    }
    mvwprintw(helper_win, 0, 0, "Helper");
    wrefresh(helper_win);
}

/*
 * init: from where to print stats.
 * end: how many files' stats we need to print.
 * win: window where we need to print.
 * if init is 0, in the first line ("..") print "total size"
 */
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

/*
 * Helper function used in show_stat: received a size,
 * it changes the unit from Kb to Mb to Gb if size > 1024 (previous unit)
 */
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
    print_border_and_title(active);
}

/*
 * Given a string str, and a line i, prints str on the I line of INFO_WIN.
 * Plus, it searches for running th, and if found, prints running_h message(depends on its type) at the end of INFO_LINE
 * It searches for selected_files too, and prints a message at the end of INFO_LINE - (strlen(running_h mesg) if there's.
 * Finally, if a search is running, prints a message at the end of ERR_LINE;
 */
void print_info(const char *str, int i)
{
    int k;
    char st[PATH_MAX], search_str[20];

    for (k = INFO_LINE; k != ERR_LINE + 1; k++) {
        wmove(info_win, k, strlen("I:") + 1);
        wclrtoeol(info_win);
    }
    k = 0;
    if (thread_h) {
        sprintf(st, "[%d/%d] %s", thread_h->num, num_of_jobs, thread_job_mesg[thread_h->type - 1]);
        k = strlen(st) + 1;
        mvwprintw(info_win, INFO_LINE, COLS - strlen(st), st);
    }
    if (selected) {
        mvwprintw(info_win, INFO_LINE, COLS - k - strlen(selected_mess), selected_mess);
    }
    if (sv.searching) {
        if (sv.searching == 3) {
            sprintf(search_str, "%d files found.", sv.found_cont);
            mvwprintw(info_win, ERR_LINE, COLS - strlen(search_str), search_str);
        } else {
            mvwprintw(info_win, ERR_LINE, COLS - strlen(searching_mess[sv.searching - 1]), searching_mess[sv.searching - 1]);
        }
    }
    if (str) {
        mvwprintw(info_win, i, strlen("I: ") + 1, str);
    }
    wrefresh(info_win);
}

/*
 * Given a str, a char input[dim], and a char c (that is default value if enter is pressed, if dim == 1),
 * asks the user "str" and saves in input the user response.
 */
void ask_user(const char *str, char *input, int dim, char c)
{
    echo();
    print_info(str, INFO_LINE);
    if (dim == 1) {
        *input = wgetch(info_win);
        if ((*input >= 'A') && (*input <= 'Z')) {
            *input = tolower(*input);
        } else if (*input == 10) {
            *input = c;
        }
    } else {
        wgetstr(info_win, input);
    }
    noecho();
    print_info(NULL, INFO_LINE);
}
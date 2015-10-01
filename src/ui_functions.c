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

static void fm_scr_init(void);
static void generate_list(int win);
static void print_border_and_title(int win);
static int is_hidden(const struct dirent *current_file);
static void initialize_tab_cwd(int win);
static void scroll_helper_func(int x, int direction);
static void colored_folders(int win, const char *name);
static void helper_print(void);
static void create_helper_win(void);
static void remove_helper_win(void);
static void change_unit(float size, char *str);
static void erase_stat(void);
static void tabs_refresh(void);
static void sig_handler(int sig_num);

static WINDOW *helper_win, *info_win, *fm[MAX_TABS];
static int dim, width[MAX_TABS], asking_question, delta[MAX_TABS], stat_active[MAX_TABS], resizing;

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
    cont = 1;
    fm_scr_init();
}

static void fm_scr_init(void)
{
    int i;

    dim = LINES - INFO_HEIGHT;
    for (i = 0; i < cont; i++) {
        new_tab(i);
        if (resizing) {
            if (ps[i].curr_pos > dim - 2 - INITIAL_POSITION) {
                delta[i] = ps[i].curr_pos - (dim - 2 - INITIAL_POSITION);
            } else {
                delta[i] = 0;
            }
            list_everything(i, delta[i], 0);
        }
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
        delete_tab(i);
    }
    if (helper_win) {
        wclear(helper_win);
        delwin(helper_win);
    }
    wclear(info_win);
    delwin(info_win);
    endwin();
    if (quit) {
        delwin(stdscr);
    }
}

/*
 * Creates a list of strings from current win path's files.
 * If needs_refresh is set to REFRESH (not FORCE_REFRESH),
 * it will check if number of files in current dir has changed.
 * If it changed, the function will create a new list of files and print them to screen (list_everything)
 * If program cannot allocate memory, it will leave.
 */
static void generate_list(int win)
{
    int i = ps[win].number_of_files, num_of_files, check = 0;
    struct dirent **files;
    char str[PATH_MAX] = {};

    num_of_files = scandir(ps[win].my_cwd, &files, is_hidden, alphasort);
    if ((ps[win].needs_refresh == FORCE_REFRESH) || (i != num_of_files)) {
        check = 1;
        free(ps[win].nl);
        ps[win].nl = NULL;
        ps[win].number_of_files = num_of_files;
        if (!(ps[win].nl = safe_malloc(sizeof(*(ps[win].nl)) * ps[win].number_of_files, fatal_mem_error))) {
            quit = 1;
        }
        if (strcmp(ps[win].my_cwd, "/") != 0) {
            strcpy(str, ps[win].my_cwd);
        }
    }
    for (i = 0; i < ps[win].number_of_files; i++) {
        if (!quit && check) {
            sprintf(ps[win].nl[i], "%s/%s", str, files[i]->d_name);
        }
        free(files[i]);
    }
    free(files);
    if (!quit && check) {
        reset_win(win);
        list_everything(win, 0, 0);
    }
}

void reset_win(int win)
{
    wclear(fm[win]);
    delta[win] = 0;
    ps[win].curr_pos = 0;
}

/*
 * Prints to window 'win' a list of strings.
 * Check if window 'win' is in search mode, and takes care.
 * If end == 0, it means it needs to print every string until the end of available rows,
 * else it indicates how many strings the function has to print (starting from files[old_dim])
 * If stat_active == 1 for 'win', and 'win' is not in search mode, it prints stats about size and permissions for every file.
 */
void list_everything(int win, int old_dim, int end)
{
    int i;
    char (*str_ptr)[PATH_MAX] = str_ptr = ps[win].nl;

    if (end == 0) {
        end = dim - 2;
    }
    if (sv.searching == 3 + win) {
        str_ptr = sv.found_searched;
    }
#if defined(LIBUDEV_PRESENT) && (SYSTEMD_PRESENT)
    else if (device_mode == 1 + win) {
        str_ptr = usb_devices;
    }
#endif
    wattron(fm[win], A_BOLD);
    for (i = old_dim; (i < ps[win].number_of_files) && (i  < old_dim + end); i++) {
        if (device_mode != 1 + win) {
            colored_folders(win, *(str_ptr + i));
        }
        if ((sv.searching == 3 + win) || (device_mode == 1 + win)) {
            mvwprintw(fm[win], INITIAL_POSITION + i - delta[win], 4, "%.*s", width[win] - 5, *(str_ptr + i));
        } else {
            mvwprintw(fm[win], INITIAL_POSITION + i - delta[win], 4, "%.*s", MAX_FILENAME_LENGTH, strrchr(*(str_ptr + i), '/') + 1);
        }
        wattroff(fm[win], COLOR_PAIR);
    }
    wattroff(fm[win], A_BOLD);
    mvwprintw(fm[win], INITIAL_POSITION + ps[win].curr_pos - delta[win], 1, "->");
    if ((sv.searching != 3 + win) && (stat_active[win] == 1)) {
        show_stat(old_dim, end, win);
    }
    print_border_and_title(win);
}

/*
 * Helper function that prints borders and title of 'win'
 */
static void print_border_and_title(int win)
{
    wborder(fm[win], '|', '|', '-', '-', '+', '+', '+', '+');
    if (sv.searching == 3 + win) {
        mvwprintw(fm[win], 0, 0, "Found file searching %.*s: ", width[win] - 1, sv.searched_string);
    }
#if defined(LIBUDEV_PRESENT) && (SYSTEMD_PRESENT)
    else if (device_mode == 1 + win) {
        mvwprintw(fm[win], 0, 0, "%.*s", width[win] - 1, device_mode_str);
    }
#endif
    else {
        mvwprintw(fm[win], 0, 0, "Current:%.*s", width[win] - 1 - strlen("Current:"), ps[win].my_cwd);
    }
    wrefresh(fm[win]);
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
 * Creates a new tab.
 * Then calculates new tab cwd, chdir there, and generate its list of files.
 */
void new_tab(int win)
{
    width[win] = COLS / cont + COLS % cont;
    fm[win] = subwin(stdscr, dim, width[win], 0, (COLS * (win)) / cont);
    keypad(fm[win], TRUE);
    scrollok(fm[win], TRUE);
    idlok(fm[win], TRUE);
    stat_active[win] = 0;
    if (!resizing) {
        initialize_tab_cwd(win);
    }
}

void restrict_first_tab(void)
{
    width[active] = COLS / cont;
    wresize(fm[active], dim, width[active]);
    print_border_and_title(active);
}

/*
 * Helper function for new_tab().
 * Saves new tab's cwd, chdir the process inside there, and put flag "needs_refresh" to FORCE_REFRESH.
 */
static void initialize_tab_cwd(int win)
{
    if (strlen(config.starting_dir)) {
        if ((cont == 1) || (config.second_tab_starting_dir)) {
            strcpy(ps[win].my_cwd, config.starting_dir);
        }
    }
    if (!strlen(ps[win].my_cwd)) {
        getcwd(ps[win].my_cwd, PATH_MAX);
    }
    ps[win].needs_refresh = FORCE_REFRESH;
}

/*
 * Removes a tab and resets its cwd
 */
void delete_tab(int win)
{
    wclear(fm[win]);
    delwin(fm[win]);
    fm[win] = NULL;
    if (!resizing) {
        free(ps[win].nl);
        ps[win].nl = NULL;
        ps[win].number_of_files = 0;
    }
}

/*
 * Called in main_loop (main.c) after deletion of second tab.
 */
void enlarge_first_tab(void)
{
    width[active] = COLS;
    wborder(fm[active], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wresize(fm[active], dim, width[active]);
    print_border_and_title(active);
}

void scroll_down(void)
{
    if (ps[active].curr_pos < ps[active].number_of_files - 1) {
        ps[active].curr_pos++;
        if (ps[active].curr_pos - (dim - 2) == delta[active]) {
            scroll_helper_func(dim - 2, 1);
            list_everything(active, ps[active].curr_pos, 1);
        } else {
            mvwprintw(fm[active], ps[active].curr_pos - delta[active], 1, "  ");
            mvwprintw(fm[active], ps[active].curr_pos - delta[active] + INITIAL_POSITION, 1, "->");
        }
    }
}

void scroll_up(void)
{
    if (ps[active].curr_pos > 0) {
        ps[active].curr_pos--;
        if (ps[active].curr_pos < delta[active]) {
            scroll_helper_func(INITIAL_POSITION, -1);
            list_everything(active, delta[active], 1);
        } else {
            mvwprintw(fm[active], ps[active].curr_pos - delta[active] + 2, 1, "  ");
            mvwprintw(fm[active], ps[active].curr_pos - delta[active] + 1, 1, "->");
        }
    }
}

static void scroll_helper_func(int x, int direction)
{
    mvwprintw(fm[active], x, 1, "  ");
    wborder(fm[active], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wscrl(fm[active], direction);
    delta[active] += direction;
}

/*
 * Follow ls color scheme to color files/folders. + Archives are yellow.
 * Receives a win and a full path to be checked.
 * If it cannot stat a file, it means it is inside an archive (in search_mode), so we print it yellow.
 */
static void colored_folders(int win, const char *name)
{
    struct stat file_stat;

    if (lstat(name, &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode)) {
            wattron(fm[win], COLOR_PAIR(1));
        } else if (S_ISLNK(file_stat.st_mode)) {
            wattron(fm[win], COLOR_PAIR(3));
        } else if ((S_ISREG(file_stat.st_mode)) && (file_stat.st_mode & S_IXUSR)) {
            wattron(fm[win], COLOR_PAIR(2));
        }
    } else {
        wattron(fm[win], COLOR_PAIR(4));
    }
}

void trigger_show_helper_message(int help)
{
    if (help) {
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
        wresize(fm[i], dim, width[i]);
        print_border_and_title(i);
        if (ps[i].curr_pos > dim - 3) {
            ps[i].curr_pos = dim - 3 + delta[i];
            mvwprintw(fm[i], dim - 3 + INITIAL_POSITION, 1, "->");
        }
        wrefresh(fm[i]);
    }
    helper_win = subwin(stdscr, HELPER_HEIGHT, COLS, LINES - INFO_HEIGHT - HELPER_HEIGHT, 0);
    wclear(helper_win);
    helper_print();
}

/*
 * Remove helper_win, resize every ps.fm win and prints last HELPER_HEIGHT lines for each ps.win.
 */
static void remove_helper_win(void)
{
    int i;

    wclear(helper_win);
    delwin(helper_win);
    helper_win = NULL;
    dim = LINES - INFO_HEIGHT;
    for (i = 0; i < cont; i++) {
        mvwhline(fm[i], dim - 1 - HELPER_HEIGHT, 0, ' ', COLS);
        wresize(fm[i], dim, width[i]);
        list_everything(i, dim - 2 - HELPER_HEIGHT + delta[i], HELPER_HEIGHT);
    }
}

static void helper_print(void)
{
    int i;

    wborder(helper_win, '|', '|', '-', '-', '+', '+', '+', '+');
    for (i = HELPER_HEIGHT - INFO_HEIGHT - 1; i >= 0; i--) {
        mvwprintw(helper_win, i + 1, 0, "| * %.*s", COLS - 5, helper_string[i]);
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
        for (i = 1; i < ps[win].number_of_files; i++) {
            stat(ps[win].nl[i], &file_stat);
            total_size += file_stat.st_size;
        }
        change_unit(total_size, str);
        mvwprintw(fm[win], INITIAL_POSITION, STAT_COL, "Total size: %s", str);
        i = 1;
    }
    for (; ((i < init + end) && (i < ps[win].number_of_files)); i++) {
        stat(ps[win].nl[i], &file_stat);
        change_unit(file_stat.st_size, str);
        mvwprintw(fm[win], i + INITIAL_POSITION - delta[win], STAT_COL, "%s", str);
        wprintw(fm[win], "\t");
        for (j = 0; j < 9; j++) {
            wprintw(fm[win], (file_stat.st_mode & perm_bit[j]) ? "%c" : "-", perm_sign[j % 3]);
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

void trigger_stats(void)
{
    stat_active[active] = !stat_active[active];
    if (stat_active[active]) {
        list_everything(active, delta[active], 0);
    } else {
        erase_stat();
    }
}

/*
 * Move to STAT_COL and clear to eol.
 */
static void erase_stat(void)
{
    int i;

    for (i = 0; (i < ps[active].number_of_files) && (i < dim - 2); i++) {
        wmove(fm[active], i + 1, STAT_COL);
        wclrtoeol(fm[active]);
    }
    print_border_and_title(active);
}

/*
 * Given a string str, and a line i, prints str on the I line of INFO_WIN.
 * Plus, if thread_h is not NULL, prints thread_job_mesg message(depends on current job type) at the end of INFO_LINE
 * It searches for selected_files too, and prints a message at the end of INFO_LINE - (strlen(running_h mesg) if there's.
 * Finally, if a search is running, prints a message at the end of ERR_LINE;
 * If asking_question != 0, there's a question being asked to user, so we won't clear it inside the for,
 * and we won't print str if i == INFO_LINE (that's the line where the user is being asked the question!)
 */
void print_info(const char *str, int i)
{
    int len = 0;
    char st[100], search_str[20];

    // "quit_with_running_thread" is the longest question string i print. If asking a question (asking_question == 1), i won't clear the question being asked.
    wmove(info_win, INFO_LINE, strlen("I:") + 1 + (asking_question * strlen(quit_with_running_thread)));
    wclrtoeol(info_win);
    wmove(info_win, ERR_LINE, strlen("E:") + 1);
    wclrtoeol(info_win);
    if (thread_h) {
        sprintf(st, "[%d/%d] %s", thread_h->num, num_of_jobs, thread_job_mesg[thread_h->type]);
        len = strlen(st) + 1;
        mvwprintw(info_win, INFO_LINE, COLS - strlen(st), st);
    }
    if (selected) {
        mvwprintw(info_win, INFO_LINE, COLS - len - strlen(selected_mess), selected_mess);
    }
    if (sv.searching) {
        if (sv.searching >= 3) {
            sprintf(search_str, "%d files found.", sv.found_cont);
            mvwprintw(info_win, ERR_LINE, COLS - strlen(search_str), search_str);
        } else {
            mvwprintw(info_win, ERR_LINE, COLS - strlen(searching_mess[sv.searching - 1]), searching_mess[sv.searching - 1]);
        }
    }
    if (str && (!asking_question || i == ERR_LINE)) {
        mvwprintw(info_win, i, strlen("I: ") + 1, "%.*s", COLS - (strlen("I: ") + 1), str);
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
    asking_question = 1;
    if (dim == 1) {
        *input = tolower(wgetch(info_win));
        if (*input == 10) {
            *input = c;
        }
    } else {
        wgetstr(info_win, input);
    }
    noecho();
    asking_question = 0;
    print_info(NULL, INFO_LINE);
}

int win_refresh_and_getch(void)
{
    tabs_refresh();
    signal(SIGUSR2, sig_handler);
    return wgetch(fm[active]);
}

static void tabs_refresh(void)
{
    int i;

    for (i = 0; i < cont; i++) {
        if ((ps[i].needs_refresh != NO_REFRESH) && (sv.searching != 3 + i) && (device_mode != 1 + i)) {
            generate_list(i);
        }
        ps[i].needs_refresh = NO_REFRESH;
    }
}

static void sig_handler(int sig_num)
{
    return tabs_refresh();
}

void resize_win(int help)
{
    resizing = 1;
    screen_end();
    fm_scr_init();
    trigger_show_helper_message(help);
    print_info(NULL, INFO_LINE);
    resizing = 0;
}
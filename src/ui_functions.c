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

static void generate_list(int win);
static void print_border_and_title(int win);
static int is_hidden(const struct dirent *current_file);
static void initialize_tab_cwd(void);
static void scroll_helper_func(int x, int direction);
static void sync_changes(void);
static void colored_folders(int win, const char *name);
static void helper_print(void);
static void create_helper_win(void);
static void remove_helper_win(void);
static void change_unit(float size, char *str);
static void erase_stat(void);

static WINDOW *helper_win, *info_win, *fm[MAX_TABS];
static int dim, width[MAX_TABS], asking_question, delta[MAX_TABS], stat_active[MAX_TABS];

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
    new_tab();
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
        delete_tab();
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
 * If needs_refresh is set to REFRESH (not FORCE_REFRESH),
 * it will check if number of files in current dir has changed.
 * If it changed, the function will create a new list of files and print them to screen (list_everything)
 * If program cannot allocate memory, it will leave.
 */
static void generate_list(int win)
{
    int i = 0, number_of_files;
    struct dirent **files;

    if (sv.searching == 3 + win) {
        return;
    }
    while ((needs_refresh == REFRESH) && (ps[win].nl[i])) {
        i++;
    }
    number_of_files = scandir(ps[win].my_cwd, &files, is_hidden, alphasort);
    if (i != number_of_files) {
        free_str(ps[win].nl);
        for (i = 0; i < number_of_files; i++) {
            if (!(ps[win].nl[i] = safe_malloc(sizeof(char) * PATH_MAX, fatal_mem_error))) {
                quit = 1;
            }
            if (strcmp(ps[win].my_cwd, "/") != 0) {
                sprintf(ps[win].nl[i], "%s/%s", ps[win].my_cwd, files[i]->d_name);
            } else {
                sprintf(ps[win].nl[i], "/%s", files[i]->d_name);
            }
        }
        reset_win(win);
        list_everything(win, 0, 0, ps[win].nl);
    }
    for (i = number_of_files - 1; i >= 0; i--) {
        free(files[i]);
    }
    free(files);
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
void list_everything(int win, int old_dim, int end, char *files[])
{
    int i;

    if (end == 0) {
        end = dim - 2;
    }
    wattron(fm[win], A_BOLD);
    for (i = old_dim; (files[i]) && (i  < old_dim + end); i++) {
        colored_folders(win, files[i]);
        if (sv.searching == 3 + win) {
            mvwprintw(fm[win], INITIAL_POSITION + i - delta[win], 4, "%.*s", width[win] - 5, files[i]);
        } else {
            mvwprintw(fm[win], INITIAL_POSITION + i - delta[win], 4, "%.*s", MAX_FILENAME_LENGTH, strrchr(files[i], '/') + 1);
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
    if (sv.searching != 3 + win) {
        mvwprintw(fm[win], 0, 0, "Current:%.*s", width[win] - 1 - strlen("Current:"), ps[win].my_cwd);
    } else {
        mvwprintw(fm[win], 0, 0, "Found file searching %.*s: ", width[active] - 1 - strlen("Found file searching : ") - strlen(search_tab_title), sv.searched_string);
        mvwprintw(fm[win], 0, width[win] - (strlen(search_tab_title) + 1), search_tab_title);
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
 * Creates a new tab and gives new tab the active flag.
 * Then calculates new tab cwd, chdir there, and generate its list of files.
 */
void new_tab(void)
{
    if (cont == MAX_TABS) {
        return;
    }
    cont++;
    if (cont == MAX_TABS) {
        width[active] = COLS / cont;
        wresize(fm[active], dim, width[active]);
        print_border_and_title(active);
    }
    active = cont - 1;
    width[active] = COLS / cont + COLS % cont;
    fm[active] = subwin(stdscr, dim, width[active], 0, (COLS * active) / cont);
    keypad(fm[active], TRUE);
    scrollok(fm[active], TRUE);
    idlok(fm[active], TRUE);
    wtimeout(fm[active], 100);
    stat_active[active] = 0;
    initialize_tab_cwd();
}

/*
 * Helper function for new_tab().
 * Saves new tab's cwd, chdir the process inside there, and put flag "needs_refresh" to FORCE_REFRESH.
 */
static void initialize_tab_cwd(void)
{
    if (config.starting_dir) {
        if ((cont == 1) || (config.second_tab_starting_dir != 0)){
            strcpy(ps[active].my_cwd, config.starting_dir);
        }
    }
    if (strlen(ps[active].my_cwd) == 0) {
        getcwd(ps[active].my_cwd, PATH_MAX);
    }
    chdir(ps[active].my_cwd);
    needs_refresh = FORCE_REFRESH;
}

/*
 * Removes a tab and resets its stored values (cwd, stat_active)
 */
void delete_tab(void)
{
    cont--;
    wclear(fm[cont]);
    delwin(fm[cont]);
    fm[cont] = NULL;
    free_str(ps[cont].nl);
    memset(ps[cont].my_cwd, 0, sizeof(ps[!active].my_cwd));
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

void scroll_down(char **str)
{
    if (str[ps[active].curr_pos + 1]) {
        ps[active].curr_pos++;
        if (ps[active].curr_pos - (dim - 2) == delta[active]) {
            scroll_helper_func(dim - 2, 1);
            list_everything(active, ps[active].curr_pos, 1, str);
        } else {
            mvwprintw(fm[active], ps[active].curr_pos - delta[active], 1, "  ");
            mvwprintw(fm[active], ps[active].curr_pos - delta[active] + INITIAL_POSITION, 1, "->");
        }
    }
}

void scroll_up(char **str)
{
    if (ps[active].curr_pos > 0) {
        ps[active].curr_pos--;
        if (ps[active].curr_pos < delta[active]) {
            scroll_helper_func(INITIAL_POSITION, -1);
            list_everything(active, delta[active], 1, str);
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
 * Calls generate_list for every tab present.
 * Put needs_refresh off.
 */
static void sync_changes(void)
{
    int i;

    for (i = 0; i < cont; i++) {
        generate_list(i);
    }
    needs_refresh = 0;
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
        } else if (is_archive(name)) {
            wattron(fm[win], COLOR_PAIR(4));
        } else if ((S_ISREG(file_stat.st_mode)) && (file_stat.st_mode & S_IXUSR)) {
            wattron(fm[win], COLOR_PAIR(2));
        }
    } else {
        wattron(fm[win], COLOR_PAIR(4));
    }
}

void trigger_show_helper_message(void)
{
    if (!helper_win) {
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
        list_everything(i, dim - 2 - HELPER_HEIGHT + delta[i], HELPER_HEIGHT, ps[i].nl);
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
        mvwprintw(fm[win], INITIAL_POSITION, STAT_COL, "Total size: %s", str);
        i = 1;
    }
    for (; ((i < init + end) && (ps[win].nl[i])); i++) {
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

void show_stats(void)
{
    stat_active[active] = !stat_active[active];
    if (stat_active[active]) {
        list_everything(active, delta[active], 0, ps[active].nl);
    } else {
        erase_stat();
    }
}

/*
 * Move to STAT_COL for each ps.fm win and clear to eol.
 */
static void erase_stat(void)
{
    int i;

    for (i = 0; (ps[active].nl[i]) && (i < dim - 2); i++) {
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
    char st[PATH_MAX], search_str[20];

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
    if (needs_refresh) {
        sync_changes();
    }
    return wgetch(fm[active]);
}
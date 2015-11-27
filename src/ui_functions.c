#include "../inc/ui_functions.h"

static void fm_scr_init(void);
static void info_win_init(void);
static void generate_list(int win);
static int sizesort(const struct dirent **d1, const struct dirent **d2);
static int last_mod_sort(const struct dirent **d1, const struct dirent **d2);
static void reset_win(int win);
static void list_everything(int win, int old_dim, int end);
static void print_border_and_title(int win);
static int is_hidden(const struct dirent *current_file);
static void initialize_tab_cwd(int win);
static void scroll_helper_func(int x, int direction);
static void colored_folders(int win, const char *name);
static void helper_print(void);
static void create_helper_win(void);
static void remove_helper_win(void);
static void show_stat(int init, int end, int win);
static void change_unit(float size, char *str);
static void erase_stat(void);
static void resize_helper_win(void);
static void resize_fm_win(void);
static void check_selected(const char *str, int win, int line);

struct scrstr {
    WINDOW *fm;
    int width;
    int delta;
    int stat_active;
    char tot_size[30];
};

static struct scrstr mywin[MAX_TABS];
static WINDOW *helper_win, *info_win;
static int dim;
static pthread_mutex_t fm_lock, info_lock;
static int (*sorting_func)(const struct dirent **d1, const struct dirent **d2) = alphasort; // file list sorting function, defaults to alphasort

/*
 * Initializes screen, colors etc etc, and calls fm_scr_init.
 */
void screen_init(void) {
    pthread_mutex_init(&fm_lock, NULL);
    pthread_mutex_init(&info_lock, NULL);
    setlocale(LC_ALL, "");
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
    dim = LINES - INFO_HEIGHT;
    if (config.starting_helper) {
        create_helper_win();
    }
    fm_scr_init();
    info_win_init();
}

/*
 * Used to initialize fm win and info_win at program startup.
 */
static void fm_scr_init(void) {
    for (int i = 0; i < cont; i++) {
        new_tab(i);
    }
}

/*
 * Initializes info_win with proper strings for every line.
 */
static void info_win_init(void) {
    info_win = subwin(stdscr, INFO_HEIGHT, COLS, LINES - INFO_HEIGHT, 0);
    keypad(info_win, TRUE);
    for (int i = 0; i < INFO_HEIGHT; i++) {
        mvwprintw(info_win, i, 1, "%s", info_win_str[i]);
    }
    wrefresh(info_win);
}
/*
 * Clear any existing window, and destroy mutexes
 */
void screen_end(void) {
    for (int i = 0; i < cont; i++) {
        delete_tab(i);
    }
    delwin(info_win);
    if (helper_win) {
        delwin(helper_win);
    }
    delwin(stdscr);
    endwin();
    pthread_mutex_destroy(&fm_lock);
    pthread_mutex_destroy(&info_lock);
}

/*
 * Creates a list of strings from current win path's files and print them to screen (list_everything)
 * If program cannot allocate memory, it will leave.
 */
static void generate_list(int win) {
    struct dirent **files;
    char str[PATH_MAX] = {0};
    
    if (mywin[win].stat_active) {
        memset(mywin[win].tot_size, 0, strlen(mywin[win].tot_size));
        mywin[win].stat_active = STATS_ON;
    }
    ps[win].number_of_files = scandir(ps[win].my_cwd, &files, is_hidden, sorting_func);
    free(ps[win].nl);
    if (!(ps[win].nl = calloc(ps[win].number_of_files, PATH_MAX))) {
        quit = MEM_ERR_QUIT;
    }
    str_ptr[win] = ps[win].nl;
    if (strcmp(ps[win].my_cwd, "/") != 0) {
        strcpy(str, ps[win].my_cwd);
    }
    for (int i = 0; i < ps[win].number_of_files; i++) {
        if (!quit) {
            sprintf(ps[win].nl[i], "%s/%s", str, files[i]->d_name);
        }
        free(files[i]);
    }
    free(files);
    if (!quit) {
        reset_win(win);
    }
}

static int sizesort(const struct dirent **d1, const struct dirent **d2) {
    struct stat stat1, stat2;
    float result;
    
    stat((*d1)->d_name, &stat1);
    stat((*d2)->d_name, &stat2);
    result = stat1.st_size - stat2.st_size;
    return (result > 0) ? -1 : 1;
}

static int last_mod_sort(const struct dirent **d1, const struct dirent **d2) {
    struct stat stat1, stat2;
    
    stat((*d1)->d_name, &stat1);
    stat((*d2)->d_name, &stat2);
    return (stat2.st_mtime - stat1.st_mtime);
}

static void reset_win(int win)
{
    wclear(mywin[win].fm);
    mywin[win].delta = 0;
    ps[win].curr_pos = 0;
    list_everything(win, 0, 0);
}

/*
 * Prints to window 'win' "end" strings, startig from old_dim.
 *  If end == 0, it means it needs to print every string until the end of available rows,
 * Checks if window 'win' is in search/device mode, and takes care.
 * If stat_active == STATS_ON for 'win', and 'win' is not in search mode, it prints stats about size and permissions for every file.
 */
static void list_everything(int win, int old_dim, int end) {
    char *str;
    int width;
    
    if (end == 0) {
        end = dim - 2;
    }
    wattron(mywin[win].fm, A_BOLD);
    for (int i = old_dim; (i < ps[win].number_of_files) && (i  < old_dim + end); i++) {
        if ((sv.searching == 3 + win) || (device_mode == 1 + win)) {
            width = mywin[win].width - 5;
            str = *(str_ptr[win] + i);
        } else {
            check_selected(*(str_ptr[win] + i), win, i);
            width = MAX_FILENAME_LENGTH;
            str = strrchr(*(str_ptr[win] + i), '/') + 1;
        }
        colored_folders(win, *(str_ptr[win] + i));
        mvwprintw(mywin[win].fm, 1 + i - mywin[win].delta, 4, "%.*s", width, str);
        wattroff(mywin[win].fm, COLOR_PAIR);
    }
    wattroff(mywin[win].fm, A_BOLD);
    mvwprintw(mywin[win].fm, 1 + ps[win].curr_pos - mywin[win].delta, 1, "->");
    if ((sv.searching != 3 + win) && (device_mode != 1 + win) && (mywin[win].stat_active == STATS_ON)) {
        show_stat(old_dim, end, win);
    }
    print_border_and_title(win);
}

/*
 * Helper function that prints borders and title of 'win'.
 * If win has stat_active == (STATS_ON || STATS_IDLE), adds current folder total size
 * to the right border's corner.
 */
static void print_border_and_title(int win) {
    wborder(mywin[win].fm, '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(mywin[win].fm, 0, 0, "%.*s", mywin[win].width - 1, ps[win].title);
    if ((sv.searching == 3 + win) || (device_mode == 1 + win)) {
        mvwprintw(mywin[win].fm, 0, mywin[win].width - strlen(special_mode_title), special_mode_title);
    } else {
        mvwprintw(mywin[win].fm, 0, mywin[win].width - strlen(mywin[win].tot_size), mywin[win].tot_size);
    }
    wrefresh(mywin[win].fm);
}

/*
 * Helper function passed to scandir (in generate_list() )
 * Will return false for '.', and for every file starting with '.' (except for '..') if !show_hidden
 */
static int is_hidden(const struct dirent *current_file) {
    if (current_file->d_name[0] == '.') {
        if ((strlen(current_file->d_name) == 1) || ((!config.show_hidden) && current_file->d_name[1] != '.')) {
            return (FALSE);
        }
    }
    return (TRUE);
}

/*
 * Creates a new tab.
 * Then, only if !resizing, calls initialize_tab_cwd().
 */
void new_tab(int win) {
    mywin[win].width = COLS / cont + COLS % cont;
    mywin[win].fm = newwin(dim, mywin[win].width, 0, (COLS * (win)) / cont);
    keypad(mywin[win].fm, TRUE);
    scrollok(mywin[win].fm, TRUE);
    idlok(mywin[win].fm, TRUE);
    initialize_tab_cwd(win);
}

/*
 * Helper functions called in main.c before creating second tab.
 */
void restrict_first_tab(void) {
    mywin[active].width = COLS / cont;
    wresize(mywin[active].fm, dim, mywin[active].width);
    print_border_and_title(active);
}

/*
 * Helper function for new_tab().
 * Calculates new tab's cwd and save new tab's title. Then refreshes UI.
 */
static void initialize_tab_cwd(int win) {
    if (strlen(config.starting_dir)) {
        if ((cont == 1) || (config.second_tab_starting_dir)) {
            strcpy(ps[win].my_cwd, config.starting_dir);
        }
    }
    if (!strlen(ps[win].my_cwd)) {
        getcwd(ps[win].my_cwd, PATH_MAX);
    }
    sprintf(ps[win].title, "%s", ps[win].my_cwd);
    tab_refresh(win);
}

/*
 * Removes a tab and if !resizing, free its list of files.
 */
void delete_tab(int win) {
    delwin(mywin[win].fm);
    mywin[win].fm = NULL;
    memset(ps[win].my_cwd, 0, sizeof(ps[win].my_cwd));
    mywin[win].stat_active = STATS_OFF;
    free(ps[win].nl);
    ps[win].nl = NULL;
}

/*
 * Called in main_loop (main.c) after deletion of second tab.
 */
void enlarge_first_tab(void) {
    mywin[active].width = COLS;
    wborder(mywin[active].fm, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wresize(mywin[active].fm, dim, mywin[active].width);
    print_border_and_title(active);
}

void scroll_down(void) {
    if (ps[active].curr_pos < ps[active].number_of_files - 1) {
        ps[active].curr_pos++;
        if (ps[active].curr_pos - (dim - 2) == mywin[active].delta) {
            scroll_helper_func(dim - 2, 1);
            if (mywin[active].stat_active == STATS_IDLE) {
                mywin[active].stat_active = STATS_ON;
            }
            list_everything(active, ps[active].curr_pos, 1);
        } else {
            mvwprintw(mywin[active].fm, ps[active].curr_pos - mywin[active].delta, 1, "  ");
            mvwprintw(mywin[active].fm, ps[active].curr_pos - mywin[active].delta + 1, 1, "->");
        }
    }
}

void scroll_up(void) {
    if (ps[active].curr_pos > 0) {
        ps[active].curr_pos--;
        if (ps[active].curr_pos < mywin[active].delta) {
            scroll_helper_func(1, -1);
            if (mywin[active].stat_active == STATS_IDLE) {
                mywin[active].stat_active = STATS_ON;
            }
            list_everything(active, mywin[active].delta, 1);
        } else {
            mvwprintw(mywin[active].fm, ps[active].curr_pos - mywin[active].delta + 2, 1, "  ");
            mvwprintw(mywin[active].fm, ps[active].curr_pos - mywin[active].delta + 1, 1, "->");
        }
    }
}

static void scroll_helper_func(int x, int direction) {
    mvwprintw(mywin[active].fm, x, 1, "  ");
    wborder(mywin[active].fm, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wscrl(mywin[active].fm, direction);
    mywin[active].delta += direction;
}

/*
 * Follows ls color scheme to color files/folders. 
 * In search mode, it highlights paths inside archives in yellow.
 * In device mode, everything is printed in yellow.
 */
static void colored_folders(int win, const char *name) {
    struct stat file_stat;

    if (lstat(name, &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode)) {
            wattron(mywin[win].fm, COLOR_PAIR(1));
        } else if (S_ISLNK(file_stat.st_mode)) {
            wattron(mywin[win].fm, COLOR_PAIR(3));
        } else if ((S_ISREG(file_stat.st_mode)) && (file_stat.st_mode & S_IXUSR)) {
            wattron(mywin[win].fm, COLOR_PAIR(2));
        }
    } else {
        wattron(mywin[win].fm, COLOR_PAIR(4));
    }
}

void trigger_show_helper_message(void) {
    pthread_mutex_lock(&fm_lock);
    if (!helper_win) {
        create_helper_win();
    } else {
        remove_helper_win();
    }
    pthread_mutex_unlock(&fm_lock);
}

/*
 * Changes "dim" global var;
 * if current position in the folder was > dim - 3 (where dim goes from 0 to dim - 1, and -2 is because of helper_win borders),
 * change it to dim - 3 + ps[i].delta.
 * Then create helper_win and print its strings.
 */
static void create_helper_win(void) {
    dim -= HELPER_HEIGHT;
    for (int i = 0; i < cont; i++) {
        wresize(mywin[i].fm, dim, mywin[i].width);
        print_border_and_title(i);
        if (ps[i].curr_pos > dim - 3 + mywin[i].delta) {
            ps[i].curr_pos = dim - 3 + mywin[i].delta;
            mvwprintw(mywin[i].fm, dim - 3 + 1, 1, "->");
        }
        wrefresh(mywin[i].fm);
    }
    helper_win = newwin(HELPER_HEIGHT, COLS, dim, 0);
    wclear(helper_win);
    helper_print();
}

/*
 * Remove helper_win, removes old bottom border of every fm win then resizes it.
 * Finally prints last HELPER_HEIGHT lines for each fm win.
 */
static void remove_helper_win(void) {
    wclear(helper_win);
    delwin(helper_win);
    helper_win = NULL;
    dim += HELPER_HEIGHT;
    for (int i = 0; i < cont; i++) {
        mvwhline(mywin[i].fm, dim - 1 - HELPER_HEIGHT, 0, ' ', COLS);
        wresize(mywin[i].fm, dim, mywin[i].width);
        if (mywin[i].stat_active == STATS_IDLE) {
            mywin[i].stat_active = STATS_ON;
        }
        list_everything(i, dim - 2 - HELPER_HEIGHT + mywin[i].delta, HELPER_HEIGHT);
    }
}

static void helper_print(void) {
    const char *title = "Press 'l' to trigger helper";
    int len = (COLS - strlen(title)) / 2;
    
    wborder(helper_win, '|', '|', '-', '-', '+', '+', '+', '+');
    for (int i = 0; i < HELPER_HEIGHT - 2; i++) {
        mvwprintw(helper_win, i + 1, 0, "| * %.*s", COLS - 5, helper_string[i]);
    }
    wattron(helper_win, A_BOLD);
    mvwprintw(helper_win, 0, len, title);
    wattroff(helper_win, A_BOLD);
    wrefresh(helper_win);
}

/*
 * init: from where to print stats.
 * end: how many files' stats we need to print. (0 means all)
 * win: window where we need to print.
 * Prints size and perms for each of the files of the win between init and init + end.
 * Plus, calculates full folder size if mywin[win].tot_size is empty (it is emptied in generate_list,
 * so it will be empty only when a full redraw of the win is needed).
 */
static void show_stat(int init, int end, int win) {
    int check = strlen(mywin[win].tot_size);
    const int perm_bit[9] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    const char perm_sign[3] = {'r', 'w', 'x'};
    char str[20];
    float total_size = 0;
    struct stat file_stat;
    
    check %= check - 1; // "check" should be 0 or 1 (strlen(tot_size) will never be 1, so i can safely divide for check - 1)
    for (int i = check * init; i < ps[win].number_of_files; i++) {
        stat(ps[win].nl[i], &file_stat);
        if (!check) {
            total_size += file_stat.st_size;
        }
        if ((i >= init) && (i < init + end)) {
            change_unit(file_stat.st_size, str);
            mvwprintw(mywin[win].fm, i + 1 - mywin[win].delta, STAT_COL, "%s\t", str);
            for (int j = 0; j < 9; j++) {
                wprintw(mywin[win].fm, (file_stat.st_mode & perm_bit[j]) ? "%c" : "-", perm_sign[j % 3]);
            }
            if ((i == init + end - 1) && (check)) {
                break;
            }
        }
    }
    if (!check) {
        change_unit(total_size, str);
        sprintf(mywin[win].tot_size, "Total size: %s", str);
    }
    mywin[win].stat_active = STATS_IDLE;
}

/*
 * Helper function used in show_stat: received a size,
 * it changes the unit from Kb to Mb to Gb if size > 1024(previous unit)
 */
static void change_unit(float size, char *str) {
    char *unit[3] = {"KB", "MB", "GB"};
    int i = 0;

    size /= 1024;
    while ((size > 1024) && (i < 3)) {
        size /= 1024;
        i++;
    }
    sprintf(str, "%.2f%s", size, unit[i]);
}

/*
 * Called when "s" is pressed, updates stat_active, then locks ui_mutex 
 * and shows stats or erase stats.
 * Then release the mutex.
 */
void trigger_stats(void) {
    mywin[active].stat_active = !mywin[active].stat_active;
    pthread_mutex_lock(&fm_lock);
    if (mywin[active].stat_active) {
        show_stat(mywin[active].delta, dim - 2, active);
    } else {
        erase_stat();
    }
    print_border_and_title(active);
    pthread_mutex_unlock(&fm_lock);
}

/*
 * Move to STAT_COL and clear to eol.
 * It deletes mywin[active].tot_size too.
 */
static void erase_stat(void) {
    for (int i = 0; (i < ps[active].number_of_files) && (i < dim - 2); i++) {
        wmove(mywin[active].fm, i + 1, STAT_COL);
        wclrtoeol(mywin[active].fm);
    }
    memset(mywin[active].tot_size, 0, strlen(mywin[active].tot_size));
}

/*
 * It locks info_lock mutex, then clears "i" line of info_win.
 * Performs various checks: if thread_h is not NULL, prints thread_job_mesg message(depends on current job type) at the end of INFO_LINE.
 * It searches for selected_files too, and prints a message at the end of INFO_LINE (beside thread_job_mesg, if present).
 * If a search is running, prints a message at the end of ERR_LINE;
 * Finally, prints str on the "i" line.
 */
void print_info(const char *str, int i) {
    char st[100] = {0};
    
    pthread_mutex_lock(&info_lock);
    wmove(info_win, i, 1 + strlen(info_win_str[i]));
    wclrtoeol(info_win);
    if (thread_h) {
        if (selected) {
            st[0] = '/';
        }
        sprintf(st + strlen(st), "[%d/%d] %s", thread_h->num, num_of_jobs, thread_job_mesg[thread_h->type]);
        mvwprintw(info_win, INFO_LINE, COLS - strlen(st), st);
    }
    if (selected) {
        mvwprintw(info_win, INFO_LINE, COLS - strlen(st) - strlen(selected_mess), selected_mess);
    }
    if (sv.searching) {
        mvwprintw(info_win, ERR_LINE, COLS - strlen(searching_mess[sv.searching - 1]), searching_mess[sv.searching - 1]);
    }
    mvwprintw(info_win, i, 1 + strlen(info_win_str[i]), "%.*s", COLS - 1, str);
    wrefresh(info_win);
    pthread_mutex_unlock(&info_lock);
}

/*
 * Given a str, a char input[d], and a char c (that is default value if enter is pressed, if dim == 1),
 * asks the user "str" and saves in input the user response.
 * It does not need its own mutex because as of now only main thread calls it.
 * I needed to replace wgetnstr function with my own wgetch cycle
 * because otherwise that would prevent KEY_RESIZE event to be managed.
 * Plus this way i can reprint the question (str) and the current answer (input) after the resize.
 * Safer: delch and addch function will lock info_lock.
 */
void ask_user(const char *str, char *input, int d, char c) {
    int s, len, i = 0;
    
    print_info(str, ASK_LINE);
    input[0] = c;
    do {
        s = wgetch(info_win);
        if (s == KEY_RESIZE) {
            resize_win();
            char resize_str[200];
            sprintf(resize_str, "%s%s", str, input);
            print_info(resize_str, ASK_LINE);
        } else if (s == 10) { // enter to exit
            break;
        } else {
            len = strlen(str) + strlen(info_win_str[ASK_LINE]) + i;
            if ((s == 127) && (i)) {    // backspace!
                input[--i] = '\0';
                pthread_mutex_lock(&info_lock);
                mvwdelch(info_win, ASK_LINE, len);
                pthread_mutex_unlock(&info_lock);
            } else if (isprint(s)) {
                if (d == 1) {
                    *input = tolower(s);
                } else {
                    sprintf(input + i, "%c", s);
                }
                i++;
                pthread_mutex_lock(&info_lock);
                mvwaddch(info_win, ASK_LINE, len + 1, s);
                pthread_mutex_unlock(&info_lock);
            }
        }
    } while (i < d);
    print_info("", ASK_LINE);
}

int win_getch(void) {
    return wgetch(mywin[active].fm);
}

/*
 * Refreshes win UI if win is not in searching or device mode.
 * Mutex is needed because worker thread can call this function too.
 */
void tab_refresh(int win) {
    if ((sv.searching != 3 + win) && (device_mode != 1 + win)) {
        pthread_mutex_lock(&fm_lock);
        generate_list(win);
        pthread_mutex_unlock(&fm_lock);
    }
}

void list_found_or_devices(int num, char (*str)[PATH_MAX], int mode) {
    pthread_mutex_lock(&fm_lock);
    ps[active].number_of_files = num;
    str_ptr[active] = str;
    if (mode == SEARCH) {
        sv.searching = 3 + active;
        sprintf(ps[active].title, "Files found searching %s:", sv.searched_string);
        sprintf(searching_mess[sv.searching - 1], "%d files found.", num);
    }
#ifdef LIBUDEV_PRESENT
    else {
        device_mode = 1 + active;
        sprintf(ps[active].title, device_mode_str);
    }
#endif
    reset_win(active);
    pthread_mutex_unlock(&fm_lock);
}

/*
 * Removes info win;
 * resizes every fm win, and moves it in the new position.
 * If helper_win != NULL, resizes it too, and moves it in the new position.
 * Then recreates info win.
 * Fixes new current_position of every fm, 
 * then prints again any "sticky" message (eg: "searching"/"pasting...")
 */
void resize_win(void) {
    pthread_mutex_lock(&info_lock);
    delwin(info_win);
    dim = LINES - INFO_HEIGHT;
    if (helper_win) {
        resize_helper_win();
    }
    resize_fm_win();
    info_win_init();
    pthread_mutex_unlock(&info_lock);
    print_info("", INFO_LINE);
}

/*
 * Just clear helper_win and resize it.
 * Then move it to the new position and print helper strings.
 */
static void resize_helper_win(void) {
    wclear(helper_win);
    dim -= HELPER_HEIGHT;
    wresize(helper_win, HELPER_HEIGHT, COLS);
    mvwin(helper_win, dim, 0);
    helper_print();
}

/*
 * Clear every fm win, resize it and move it to its new position.
 * Fix new win's delta with new available LINES (ie: dim).
 * Then list_everything, being careful if stat_active is ON.
 */
static void resize_fm_win(void) {
    pthread_mutex_lock(&fm_lock);
    for (int i = 0; i < cont; i++) {
        wclear(mywin[i].fm);
        mywin[i].width = COLS / cont + i * (COLS % cont);
        wresize(mywin[i].fm, dim, mywin[i].width);
        mvwin(mywin[i].fm, 0, i * mywin[i].width);
        if (ps[i].curr_pos > dim - 3) {
            mywin[i].delta = ps[i].curr_pos - (dim - 3);
        } else {
            mywin[i].delta = 0;
        }
        if (mywin[i].stat_active == STATS_IDLE) {
            mywin[i].stat_active = STATS_ON;
        }
        list_everything(i, mywin[i].delta, 0);
    }
    pthread_mutex_unlock(&fm_lock);
}

void change_sort(void) {
    if (sorting_func == alphasort) {
        sorting_func = sizesort;
        print_info("Files will be sorted by size now.", INFO_LINE);
    } else if (sorting_func == sizesort) {
        sorting_func = last_mod_sort;
        print_info("Files will be sorted by last access now.", INFO_LINE);
    } else {
        sorting_func = alphasort;
        print_info("Files will be sorted alphabetically now.", INFO_LINE);
    }
    for (int i = 0; i < cont; i++) {
        tab_refresh(i);
    }
}

/*
 * Called in manage_space_press() (fm_functions.c)
 * It checks for each fm win, if they're on the same cwd, then checks
 * if the file to be highlighted is visible inside "win" -> useful only if win is not the active one.
 * Then prints c char before current filename and refreshes win.
 */
void highlight_selected(int line, const char c) {
    for (int i = 0; i < cont; i++) {
        if ((sv.searching != 3 + i) && (device_mode != 1 + i)) {
            if ((i == active) || ((strcmp(ps[i].my_cwd, ps[active].my_cwd) == 0) 
            && (line - mywin[i].delta > 0) && (line - mywin[i].delta < dim - 2))) {
                wattron(mywin[i].fm, A_BOLD);
                mvwprintw(mywin[i].fm, 1 + line - mywin[i].delta, SEL_COL, "%c", c);
                wattroff(mywin[i].fm, A_BOLD);
                wrefresh(mywin[i].fm);
            }
        }
    }
}

static void check_selected(const char *str, int win, int line) {
    file_list *tmp = selected;
    
    while (tmp) {
        if (strcmp(tmp->name, str) == 0) {
            mvwprintw(mywin[win].fm, 1 + line - mywin[win].delta, SEL_COL, "*");
            break;
        }
        tmp = tmp->next;
    }
}

void erase_selected_highlight(void) {
    for (int j = 0; j < cont; j++) {
        for (int i = 0; (i < dim - 2) && (i < ps[j].number_of_files); i++) {
            mvwprintw(mywin[j].fm, 1 + i, SEL_COL, " ");
        }
        wrefresh(mywin[j].fm);
    }
}
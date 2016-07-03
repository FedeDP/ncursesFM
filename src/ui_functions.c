#include "../inc/ui_functions.h"

static void info_win_init(void);
static void generate_list(int win);
static int sizesort(const struct dirent **d1, const struct dirent **d2);
static int last_mod_sort(const struct dirent **d1, const struct dirent **d2);
static int typesort(const struct dirent **d1, const struct dirent **d2);
static void list_everything(int win, int old_dim, int end);
static void print_arrow(int win);
static void check_active(int win);
static void print_border_and_title(int win);
static int is_hidden(const struct dirent *current_file);
static void initialize_tab_cwd(int win);
static void scroll_helper_func(int x, int direction, int win);
static void colored_folders(WINDOW *win, const char *name);
static void helper_print(void);
static void trigger_show_additional_win(int height, WINDOW **win, void (*f)(void));
static void create_additional_win(int height, WINDOW **win, void (*f)(void));
static void remove_additional_win(int height, WINDOW **win, int resizing);
static void show_stat(int init, int end, int win);
static void erase_stat(void);
static void info_print(const char *str, int i);
static void fix_input_cursor_pos(void);
#if ARCHIVE_VERSION_NUMBER >= 3002000
static void archiver_cb_func(void);
#endif
static void sig_handler(int fd);
static void info_refresh(int fd);
static void inotify_refresh(int win);
static int print_additional_wins(int helper_height, int resizing);
static void resize_fm_win(void);
static void check_selected(const char *str, int win, int line);
static int check_sysinfo_where(int where, int len);
static void fullname_print(void);
static void update_fullname_win(void);

/*
 * struct written to the pipe2.
 */
struct info_msg {
    char *msg;
    uint8_t line;
};

static WINDOW *helper_win, *info_win, *fullname_win;
static int dim, hidden, fullname_win_height, input_mode, input_cursor_pos;
size_t input_len;
static int (*const sorting_func[])(const struct dirent **d1, const struct dirent **d2) = {
    alphasort, sizesort, last_mod_sort, typesort
};

/*
 * Initializes screen, colors etc etc.
 */
void screen_init(void) {
    initscr();
    start_color();
    use_default_colors();
    init_pair(1, COLOR_BLUE, -1);
    init_pair(2, COLOR_CYAN, -1);
    init_pair(3, COLOR_GREEN, -1);
    init_pair(4, COLOR_YELLOW, -1);
    init_pair(5, COLOR_RED, -1);
    noecho();
    curs_set(0);
    mouseinterval(0);
    ESCDELAY = 25;
    raw();
    nodelay(stdscr, TRUE);
    notimeout(stdscr, TRUE);
    dim = LINES - INFO_HEIGHT;
    if (config.starting_helper) {
        trigger_show_helper_message();
    }
    info_win_init();
    cont = 1;
    new_tab(cont - 1);
}

/*
 * Initializes info_win with proper strings for every line.
 */
static void info_win_init(void) {
    info_win = subwin(stdscr, INFO_HEIGHT, COLS, LINES - INFO_HEIGHT, 0);
    keypad(info_win, TRUE);
    nodelay(info_win, TRUE);
    notimeout(info_win, TRUE);
    for (int i = 0; i < INFO_HEIGHT - 1; i++) {     /* -1 because SYSINFO line has its own init func (timer_func) */
        print_info("", i);
    }
    timer_func();
}
/*
 * Clear any existing window, and close info_pipe
 */
void screen_end(void) {
    if (stdscr) {
        for (int i = 0; i < cont; i++) {
            delete_tab(i);
        }
        delwin(info_win);
        /*
         * needed: this way any other call to print_info won't do anything:
         * to avoid any print_info call by some worker thread
         * while we're leaving/we left the program.
         */
        info_win = NULL;
        if (helper_win) {
            delwin(helper_win);
        }
        if (fullname_win) {
            delwin(fullname_win);
        }
        delwin(stdscr);
        endwin();
    }
}

/*
 * Creates a list of strings from current win path's files and print them to screen (list_everything)
 * If program cannot allocate memory, it will leave.
 */
static void generate_list(int win) {
    struct dirent **files;
    
    hidden = ps[win].show_hidden;
    ps[win].number_of_files = scandir(ps[win].my_cwd, &files, is_hidden, sorting_func[ps[win].sorting_index]);
    free(ps[win].nl);
    if (!(ps[win].nl = calloc(ps[win].number_of_files, PATH_MAX))) {
        quit = MEM_ERR_QUIT;
        ERROR("could not malloc. Leaving.");
    }
    str_ptr[win] = ps[win].nl;
    for (int i = 0; i < ps[win].number_of_files; i++) {
        if (!quit) {
            snprintf(ps[win].nl[i], PATH_MAX, "%s/%s", ps[win].my_cwd, files[i]->d_name);
        }
        free(files[i]);
    }
    free(files);
    if (!quit) {
        reset_win(win);
    }
}

/*
 * Callback function to scandir: list files by size.
 */
static int sizesort(const struct dirent **d1, const struct dirent **d2) {
    struct stat stat1, stat2;
    float result;

    stat((*d1)->d_name, &stat1);
    stat((*d2)->d_name, &stat2);
    result = stat1.st_size - stat2.st_size;
    return (result > 0) ? -1 : 1;
}

/*
 * Callback function to scandir: list files by last modified.
 */
static int last_mod_sort(const struct dirent **d1, const struct dirent **d2) {
    struct stat stat1, stat2;

    stat((*d1)->d_name, &stat1);
    stat((*d2)->d_name, &stat2);
    return (stat2.st_mtime - stat1.st_mtime);
}

/*
 * Callback function to scandir: list files by type.
 */
static int typesort(const struct dirent **d1, const struct dirent **d2) {
    int ret;

    if ((*d1)->d_type == (*d2)->d_type) {
        return alphasort(d1, d2);
    }
    if ((*d1)->d_type == DT_DIR) {
        ret = -1;
    } else if (((*d1)->d_type == DT_REG) && ((*d2)->d_type == DT_LNK)) {
        ret = -1;
    } else {
        ret = 1;
    }
    return ret;
}

/*
 * Clear tab, reset every var, if stat_active was idle, turn it on,
 * then call list_everything.
 */
void reset_win(int win) {
    wclear(ps[win].mywin.fm);
    ps[win].mywin.delta = 0;
    ps[win].curr_pos = 0;
    memset(ps[win].mywin.tot_size, 0, strlen(ps[win].mywin.tot_size));
    list_everything(win, 0, dim - 2);
}

/*
 * Prints to window 'win' "end" strings, startig from old_dim.
 * If end == 0, it means it needs to print every string until the end of available rows,
 * If stat_active == STATS_ON for 'win', and 'win' is not in special_mode, 
 * it prints stats about size and permissions for every file.
 */
static void list_everything(int win, int old_dim, int end) {
    char *str;
    wchar_t name[NAME_MAX + 1] = {0};
    
    wattron(ps[win].mywin.fm, A_BOLD);
    for (int i = old_dim; (i < ps[win].number_of_files) && (i  < old_dim + end); i++) {
        wmove(ps[win].mywin.fm, i + 1 - ps[win].mywin.delta, 1);
        wclrtoeol(ps[win].mywin.fm);
        if (ps[win].mode > fast_browse_) {
            str = *(str_ptr[win] + i);
        } else {
            check_selected(*(str_ptr[win] + i), win, i);
            str = strrchr(*(str_ptr[win] + i), '/') + 1;
        }
        colored_folders(ps[win].mywin.fm, *(str_ptr[win] + i));
        mbstowcs(name, str, NAME_MAX);
        mvwprintw(ps[win].mywin.fm, 1 + i - ps[win].mywin.delta, 4, "%.*ls", ps[win].mywin.width - 5, name);
        wattroff(ps[win].mywin.fm, COLOR_PAIR);
    }
    wattroff(ps[win].mywin.fm, A_BOLD);
    if (ps[win].mywin.stat_active) {
        show_stat(old_dim, end, win);
    }
    print_arrow(win);
    print_border_and_title(win);
}

static void print_arrow(int win) {
    check_active(win);
    mvwprintw(ps[win].mywin.fm, 1 + ps[win].curr_pos - ps[win].mywin.delta, 1, "%ls", config.cursor_chars);
    wattroff(ps[win].mywin.fm, COLOR_PAIR);
    wattroff(ps[win].mywin.fm, A_BOLD);
    if (fullname_win && win == active) {
        update_fullname_win();
    }
}

static void check_active(int win) {
    if (active == win) {
        if (ps[win].mode == fast_browse_) {
            wattron(ps[win].mywin.fm, A_BOLD | COLOR_PAIR(FAST_BROWSE_COL));
        } else {
            wattron(ps[win].mywin.fm, A_BOLD | COLOR_PAIR(ACTIVE_COL));
        }
    }
}

/*
 * Helper function that prints borders and title of 'win'.
 * to the right border's corner.
 */
static void print_border_and_title(int win) {
    if (ps[win].mywin.fm) {
        check_active(win);
        wborder(ps[win].mywin.fm, 0, 0, 0, 0, 0, 0 , 0 , 0);
        mvwprintw(ps[win].mywin.fm, 0, 0, "%.*s", ps[win].mywin.width - 1, _(ps[win].title));
        mvwprintw(ps[win].mywin.fm, 0, ps[win].mywin.width - strlen(ps[win].mywin.tot_size), ps[win].mywin.tot_size);
        wattroff(ps[win].mywin.fm, COLOR_PAIR);
        wattroff(ps[win].mywin.fm, A_BOLD);
        wrefresh(ps[win].mywin.fm);
    }
}

/*
 * Helper function passed to scandir (in generate_list() )
 * Will return false for '.', and for every file starting with '.' (except for '..') if !show_hidden
 */
static int is_hidden(const struct dirent *current_file) {
    if (current_file->d_name[0] == '.') {
        if ((strlen(current_file->d_name) == 1) || ((!hidden) && current_file->d_name[1] != '.')) {
            return (FALSE);
        }
    }
    return (TRUE);
}

/*
 * Creates a new tab with right attributes.
 * Then calls initialize_tab_cwd().
 */
void new_tab(int win) {
    ps[win].mywin.width = COLS / cont + win * (COLS % cont);
    ps[win].mywin.fm = newwin(dim, ps[win].mywin.width, 0, (COLS * win) / cont);
    keypad(ps[win].mywin.fm, TRUE);
    scrollok(ps[win].mywin.fm, TRUE);
    idlok(ps[win].mywin.fm, TRUE);
    notimeout(ps[win].mywin.fm, TRUE);
    nodelay(ps[win].mywin.fm, TRUE);
    initialize_tab_cwd(win);
}

/*
 * Resizes "win" tab and moves it to its new position.
 */
void resize_tab(int win, int resizing) {
    wclear(ps[win].mywin.fm);
    ps[win].mywin.width = COLS / cont + win * (COLS % cont);
    wresize(ps[win].mywin.fm, dim, ps[win].mywin.width);
    mvwin(ps[win].mywin.fm, 0, (COLS * win) / cont);
    if (resizing) {
        if (ps[win].curr_pos > dim - 3) {
            ps[win].mywin.delta = ps[win].curr_pos - (dim - 3);
        } else {
            ps[win].mywin.delta = 0;
        }
    }
    list_everything(win, ps[win].mywin.delta, dim - 2);
}

/*
 * Helper function for new_tab().
 * Calculates new tab's cwd and saves new tab's title.
 * Add an inotify_watcher on the new tab's cwd.
 * Then refreshes UI.
 */
static void initialize_tab_cwd(int win) {
    if (strlen(config.starting_dir)) {
        if ((cont == 1) || (config.second_tab_starting_dir)) {
            strncpy(ps[win].my_cwd, config.starting_dir, PATH_MAX);
        }
    }
    if (!strlen(ps[win].my_cwd)) {
        getcwd(ps[win].my_cwd, PATH_MAX);
    }
    ps[win].old_file[0] = 0;
    ps[win].show_hidden = config.show_hidden;
    if (change_dir(ps[win].my_cwd, win) == -1) {
        quit = GENERIC_ERR_QUIT;
        ERROR("could not scan current dir. Leaving.");
    }
}

/*
 * Removes a tab, reset its attributes,
 * frees its list of files and removes its inotify watcher.
 */
void delete_tab(int win) {
    delwin(ps[win].mywin.fm);
    ps[win].mywin.fm = NULL;
    memset(ps[win].my_cwd, 0, sizeof(ps[win].my_cwd));
    memset(ps[win].mywin.tot_size, 0, strlen(ps[win].mywin.tot_size));
    ps[win].mywin.stat_active = 0;
    ps[win].mode = normal;
    free(ps[win].nl);
    inotify_rm_watch(ps[win].inot.fd, ps[win].inot.wd);
    ps[win].nl = NULL;
}

void scroll_down(int win, int lines) {
    int delta = ps[win].mywin.delta;
    int old_pos = ps[win].curr_pos;
    
    while (lines > 0) {
        if (ps[win].curr_pos < ps[win].number_of_files - 1) {
            ps[win].curr_pos++;
            if (ps[win].curr_pos - (dim - 2) == ps[win].mywin.delta) {
                ps[win].mywin.delta++;
            }
        }
        lines--;
    }
    if (ps[win].mywin.delta > delta) {
        delta = ps[win].mywin.delta - delta;
        scroll_helper_func(dim - 2, delta, win);
        list_everything(win, ps[win].curr_pos - delta + 1, delta);
    } else {
        mvwprintw(ps[win].mywin.fm, old_pos - ps[win].mywin.delta + 1, 1, "  ");
        print_arrow(win);
        wrefresh(ps[win].mywin.fm);
    }
}

void scroll_up(int win, int lines) {
    int delta = ps[win].mywin.delta;
    int old_pos = ps[win].curr_pos;
    
    while (lines > 0) {
        if (ps[win].curr_pos > 0) {
            ps[win].curr_pos--;
            if (ps[win].curr_pos < ps[win].mywin.delta) {
                ps[win].mywin.delta--;
            }
        }
        lines--;
    }
    if (delta > ps[win].mywin.delta) {
        delta = delta - ps[win].mywin.delta;
        scroll_helper_func(1, -delta, win);
        list_everything(win, ps[win].mywin.delta, delta);
    } else {
        mvwprintw(ps[win].mywin.fm, old_pos - ps[win].mywin.delta + 1, 1, "  ");
        print_arrow(win);
        wrefresh(ps[win].mywin.fm);
    }
}

static void scroll_helper_func(int x, int direction, int win) {
    mvwprintw(ps[win].mywin.fm, x, 1, "  ");
    wborder(ps[win].mywin.fm, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wscrl(ps[win].mywin.fm, direction);
}

/*
 * Follows ls color scheme to color files/folders.
 * In search mode, it highlights paths inside archives in yellow.
 * In device mode, everything is printed in yellow.
 */
static void colored_folders(WINDOW *win, const char *name) {
    struct stat file_stat;

    if (lstat(name, &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode)) {
            wattron(win, COLOR_PAIR(1));
        } else if (S_ISLNK(file_stat.st_mode)) {
            wattron(win, COLOR_PAIR(2));
        } else if ((S_ISREG(file_stat.st_mode)) && (file_stat.st_mode & S_IXUSR)) {
            wattron(win, COLOR_PAIR(3));
        }
    } else {
        wattron(win, COLOR_PAIR(4));
    }
}

static void trigger_show_additional_win(int height, WINDOW **win, void (*f)(void)) {
    if (!(*win)) {
        if (dim - height >= 3) {
            create_additional_win(height, win, f);
        } else {
            print_info(_(win_too_small), ERR_LINE);
        }
    } else {
        remove_additional_win(height, win, 0);
    }
}

/*
 * Changes "dim" global var;
 * if current position in the folder was > dim - 3 (where dim goes from 0 to dim - 1, and -2 is because of helper_win borders),
 * change it to dim - 3 + ps[i].delta.
 * Then create helper_win and print its strings.
 */
static void create_additional_win(int height, WINDOW **win, void (*f)(void)) {
    dim -= height;
    for (int i = 0; i < cont; i++) {
        wresize(ps[i].mywin.fm, dim, ps[i].mywin.width);
        if (ps[i].curr_pos > dim - 3 + ps[i].mywin.delta) {
            int delta = ps[i].curr_pos - (dim - 3 + ps[i].mywin.delta);
            ps[i].curr_pos = dim - 3 + ps[i].mywin.delta;
            scroll_down(i, delta);
        } else {
            print_border_and_title(i);
        }
    }
    *win = newwin(height, COLS, dim, 0);
    wclear(*win);
    f();
    wrefresh(*win);
}

/*
 * Remove helper_win, removes old bottom border of every fm win then resizes it.
 * Finally prints last HELPER_HEIGHT lines for each fm win.
 */
static void remove_additional_win(int height, WINDOW **win, int resizing) {
    wclear(*win);
    delwin(*win);
    *win = NULL;
    dim += height;
    for (int i = 0; i < cont; i++) {
        mvwhline(ps[i].mywin.fm, dim - 1 - height, 0, ' ', COLS);
        wresize(ps[i].mywin.fm, dim, ps[i].mywin.width);
        if (!resizing) {
            list_everything(i, dim - 2 - height + ps[i].mywin.delta, height);
        }
    }
}

void trigger_show_helper_message(void) {
    int fullname_win_needed = 0;
    /*
     * If there's already fullname_win, remove it,
     * then recreate it after helper_win has been created.
     * This is to avoid an issue where helper_win would go below
     * fullname_win, but fullname_win at next refresh would go above it.
     */
    if (fullname_win && (dim - HELPER_HEIGHT[ps[active].mode] >= 3)) {
        remove_additional_win(fullname_win_height, &fullname_win, 0);
        fullname_win_needed = 1;
    }
    trigger_show_additional_win(HELPER_HEIGHT[ps[active].mode], &helper_win, helper_print);
    if (fullname_win_needed) {
        trigger_show_additional_win(fullname_win_height, &fullname_win, fullname_print);
    }
}

static void helper_print(void) {
    char ncursesFM[40] = "NcursesFM ";

    wborder(helper_win, 0, 0, 0, 0, 0, 0 , 0 , 0);
    for (int i = 0; i < HELPER_HEIGHT[ps[active].mode] - 2; i++) {
        mvwprintw(helper_win, i + 1, 2, "* %.*s", COLS - 5, _(helper_string[ps[active].mode][i]));
    }
    sprintf(ncursesFM + strlen(ncursesFM), "%s", VERSION);
    wattron(helper_win, A_BOLD);
    mvwprintw(helper_win, 0, COLS - strlen(ncursesFM), "%.*s", COLS, ncursesFM);
    mvwprintw(helper_win, 0, 0, "%.*s", COLS, _(helper_title));
    wattroff(helper_win, A_BOLD);
}

/*
 * init: from where to print stats.
 * end: how many files' stats we need to print. (0 means all)
 * win: window where we need to print.
 * Prints size and perms for each of the files of the win between init and init + end.
 * Plus, calculates full folder size if ps[win].mywin.tot_size is empty (it is emptied in generate_list,
 * so it will be empty only when a full redraw of the win is needed).
 */
static void show_stat(int init, int end, int win) {
    int check = strlen(ps[win].mywin.tot_size);
    const int perm_bit[9] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    const char perm_sign[3] = {'r', 'w', 'x'};
    char str[100] = {0};
    float total_size = 0;
    struct stat file_stat;
    const int perm_col = ps[win].mywin.width - PERM_LENGTH;
    const int size_col = ps[win].mywin.width - STAT_LENGTH;
    int col;
    
    if (ps[win].mode <= fast_browse_) {
        check %= check - 1; // "check" should be 0 or 1 (strlen(tot_size) will never be 1, so i can safely divide for check - 1)
    } else {
        check = 1;  // if we're in special mode, we don't need printing total size.
    }
    for (int i = check * init; i < ps[win].number_of_files; i++) {
        if (stat(str_ptr[win][i], &file_stat) == -1 && ps[win].mode != device_) {
            continue;
        }
        if (!check) {
            total_size += file_stat.st_size;
        }
        if ((i >= init) && (i < init + end)) {
            if (ps[win].mode == device_) {
#ifdef SYSTEMD_PRESENT
                show_devices_stat(i, win, str);
                col = ps[win].mywin.width - strlen(str) - cont;
                if (col < 0) {
                    col = 4;
                }
#endif
            } else {
                col = size_col;
                change_unit(file_stat.st_size, str);
            }
            // if show_devices_stat returned a non-empty string
            // or we are not in device_mode
            if (strlen(str)) {
                wmove(ps[win].mywin.fm, i + 1 - ps[win].mywin.delta, col);
                wclrtoeol(ps[win].mywin.fm);
                mvwprintw(ps[win].mywin.fm, i + 1 - ps[win].mywin.delta, col, "%.*s", ps[win].mywin.width - 5, str);
            }
            if (ps[win].mode != device_) {
                for (int j = 0; j < 9; j++) {
                    mvwprintw(ps[win].mywin.fm, i + 1 - ps[win].mywin.delta, perm_col + j, 
                              (file_stat.st_mode & perm_bit[j]) ? "%c" : "-", perm_sign[j % 3]);
                }
            }
            if ((i == init + end - 1) && (check)) {
                break;
            }
        }
    }
    if (!check) {
        change_unit(total_size, str);
        sprintf(ps[win].mywin.tot_size, "Total size: %s", str);
    }
}

/*
 * Called when "s" is pressed, updates stat_active,
 * then shows stats or erase stats.
 */
void trigger_stats(void) {
    ps[active].mywin.stat_active = !ps[active].mywin.stat_active;
    if (ps[active].mywin.stat_active) {
        show_stat(ps[active].mywin.delta, dim - 2, active);
    } else {
        erase_stat();
    }
    print_border_and_title(active);
}

/*
 * Move to STAT_COL and clear to eol.
 * It deletes ps[active].mywin.tot_size too.
 */
static void erase_stat(void) {
    memset(ps[active].mywin.tot_size, 0, strlen(ps[active].mywin.tot_size));
    list_everything(active, ps[active].mywin.delta, dim - 2);
}

/*
 * Clears i line to the end, then prints str string.
 * Then performs some checks about some "sticky messages"
 * (eg: "Pasting..." while a thread is pasting a file)
 */
static void info_print(const char *str, int i) {
    char st[100] = {0};

    wmove(info_win, i, 1);
    wclrtoeol(info_win);
    mvwprintw(info_win, i, 1, info_win_str[i]);
    wattron(info_win, A_BOLD | COLOR_PAIR(i + 3));
    wprintw(info_win, "%.*s", COLS - strlen(info_win_str[i]) - 1, str);
    wattroff(info_win, A_BOLD | COLOR_PAIR(i + 3));
    if (i == INFO_LINE) {
        if (selected) {
            strncpy(st, _(selected_mess), sizeof(st) - 1);
        }
        if (thread_h) {
            sprintf(st + strlen(st), "[%d/%d] %s", thread_h->num, num_of_jobs, _(thread_job_mesg[thread_h->type]));
        }
        mvwprintw(info_win, INFO_LINE, COLS - strlen(st), st);
    } else if ((i == ERR_LINE) && (sv.searching)) {
        mvwprintw(info_win, ERR_LINE, COLS - strlen(_(searching_mess[sv.searching - 1])), _(searching_mess[sv.searching - 1]));
    }
    wrefresh(info_win);
}

/*
 * if info_win is not NULL:
 * it needs to malloc (COLS - len) bytes as they're real printable chars on the screen.
 * we need malloc because window can be resized (ie: COLS is not a constant)
 * then writes on the pipe the address of the heap-allocated struct info_msg.
 */
void print_info(const char *str, int line) {
    if (info_win) {
        struct info_msg *info;
        int len = 1 + strlen(info_win_str[line]);
    
        if (!(info = malloc(sizeof(struct info_msg)))) {
            goto error;
        }
        if (!(info->msg = malloc(sizeof(char) * (COLS - len)))) {
            free(info);
            goto error;
        }
        strncpy(info->msg, str, COLS - len);
        info->line = line;
        ssize_t r = write(info_fd[1], &info, sizeof(struct info_msg *));
        if (r <= 0) {
            free(info->msg);
            free(info);
            WARN("a message could not be written.");
        }
    }
    return;
    
error:
    quit = MEM_ERR_QUIT;
    ERROR("could not malloc.");
}

void print_and_warn(const char *err, int line) {
    print_info(err, line);
    WARN(err);
}

/*
 * Given a str, a char input[d], and a char c (that is default value if enter is pressed, if dim == 1),
 * asks the user "str" and saves in input the user response.
 * I needed to replace wgetnstr function with my own wgetch cycle
 * because otherwise that would prevent KEY_RESIZE event to be managed.
 * Plus this way i can reprint the question (str) and the current answer (input) after the resize.
 * 'd' should always be an upper bound very large, so we will never lose chars while converting
 * wchar to char in  wcstombs(input, wstring, d)
 */
void ask_user(const char *str, char *input, int d) {
    int leave = 0, index = 0, overtype_mode = 0;
    wchar_t wstring[d + 1]; // space for terminating null char in case d == 1
    wchar_t wquest[100];
    char resize_str[d + 1 + strlen(str)];
    
    wmemset(wstring, 0, d + 1);
    memset(input, 0, d);
    input_cursor_pos = 0;
    
    // needed as question can have unicode chars
    mbstowcs(wquest, str, 100);
    input_len = wcswidth(wquest, 100) + strlen(info_win_str[ASK_LINE]);
    
    print_info(str, ASK_LINE);
    curs_set(1);
    input_mode = 1;
#if ARCHIVE_VERSION_NUMBER >= 3002000
    // avoid getting other ask_user calls from archiver_cb_func
    // while already asking another question.
    main_p[ARCHIVE_IX].fd = -1;
#endif
    
    MEVENT event;
    
     while ((wcslen(wstring) < d) && (!quit) && (!leave)) {
        wint_t s = main_poll(info_win);
        switch (s) {
        case KEY_RESIZE:
            resize_win();
            // here we don't need to use wquest because
            // we're only appending bytes to a multibytes string, to
            // let ncurses print resize_str.
            strcpy(resize_str, str);
            wcstombs(resize_str + strlen(resize_str), wstring, d);
            print_info(resize_str, ASK_LINE);
            break;
        case 10: // enter to return
            leave = 1;
            break;
        case 127: case KEY_BACKSPACE: // backspace
            if (index > 0) {
                index--;
                input_cursor_pos -= wcwidth(wstring[index]);
                wmemmove(&wstring[index], &wstring[index + 1], wcslen(wstring) - index);
                wmove(info_win, ASK_LINE, input_len + input_cursor_pos + 1);
                wclrtoeol(info_win);
                mvwaddwstr(info_win, ASK_LINE, input_len + input_cursor_pos + 1, wstring + index);
            }
            break;
        case KEY_LEFT:
            if (index > 0) {
                index--;
                input_cursor_pos -= wcwidth(wstring[index]);
            }
            break;
        case KEY_RIGHT:
            if (index < wcslen(wstring)) {
                input_cursor_pos += wcwidth(wstring[index]);
                index++;
            }
            break;
        case KEY_DC: // del 
            if (index < wcslen(wstring)) {
                wmemmove(&wstring[index], &wstring[index + 1],  wcslen(wstring) - index);
                wclrtoeol(info_win);
                mvwaddwstr(info_win, ASK_LINE, input_len + index + 1, wstring + index);
            }
            break;
        // why are they wprintable??
        case KEY_UP: case KEY_DOWN:
        case KEY_PPAGE:  case KEY_NPAGE:
            break;
        case KEY_MOUSE:
            if(getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_RELEASED) {
                    /* left click will send an enter event */
                    leave = 1;
                } else if (event.bstate & BUTTON3_RELEASED) {
                    /* right click will send a ESC event */
                    wmemset(wstring, 0,  wcslen(wstring));
                    input[0] = 27;
                    leave = 1;
                }
                /* scroll back and forth string events associated with mouse wheel */
#if NCURSES_MOUSE_VERSION > 1
                else if (event.bstate & BUTTON4_PRESSED) {
                    if (index > 0) {
                        index--;
                        input_cursor_pos -= wcwidth(wstring[index]);
                    }
                } else if (event.bstate & BUTTON5_PRESSED) {
                    if (index < wcslen(wstring)) {
                        input_cursor_pos += wcwidth(wstring[index]);
                        index++;
                    }
                }
#endif
            }
            break;
        case 27:   // ESC to leave input mode
            // return a string with ESC as only char
            wmemset(wstring, 0,  wcslen(wstring));
            input[0] = 27;
            leave = 1;
            break;
        case KEY_IC:
            overtype_mode = !overtype_mode;
            print_info(_(insert_string[overtype_mode]), INFO_LINE);
            curs_set(1 + overtype_mode);
            break;
        case KEY_HOME:
            index = 0;
            input_cursor_pos = 0;
            break;
        case KEY_END:
            index = wcslen(wstring);
            input_cursor_pos = wcswidth(wstring, d);
            break;
        default:
            if (iswprint(s)) {
                if (d == 1) {
                    s = towlower(s);
                    wstring[0] = s;
                    // no need to print single char as it will immediately be returned and ASK_LINE cleared
                } else {
                    if (overtype_mode && index < wcslen(wstring)) {
                        wstring[index] = s;
                        wclrtoeol(info_win);
                        mvwaddwstr(info_win, ASK_LINE, input_len + index + 1, wstring + index);
                    } else {
                        wmemmove(&wstring[index + 1], &wstring[index],  wcslen(wstring) - index);
                        wstring[index] = s;
                        waddwstr(info_win, wstring + index);
                    }
                    input_cursor_pos += wcwidth(wstring[index]);
                    index++;
                }
            }
            break;
        }
    }
    if (quit) {
        wmemset(wstring, 0,  wcslen(wstring));
        input[0] = 27;
    }
    if (wcslen(wstring) > 0) {
        wcstombs(input, wstring, d);
    }
    curs_set(0);
    input_mode = 0;
#if ARCHIVE_VERSION_NUMBER >= 3002000
    if (quit) {
        /*
         * if we're quitting, try to unlock
         * extractor thread (if any) before leaving.
         * otherwise it could stop on its eventfd read
         * eg: extracting thread is stopped on read on eventfd, 
         * but main thread is in another ask_user, and we receive a SIGTERM/SIGINT,
         * main thread will leave ask_user, but extracting thread will be blocked on eventfd.
         */
        archiver_cb_func();
    } else {
        // return to listen to archive_cb ask_user events as we now finished.
        main_p[ARCHIVE_IX].fd = archive_cb_fd[0];
    }
#endif
    mvwchgat(info_win, 0, 1, -1, A_NORMAL, 0, NULL);
    print_info("", ASK_LINE); // clear ASK_LINE
}

static void fix_input_cursor_pos(void) {
    if (input_mode) {
        mvwchgat(info_win, 0, 1, -1, A_BOLD, 3, NULL);
        wmove(info_win, ASK_LINE, input_len + input_cursor_pos + 1);
        wrefresh(info_win);
    }
}

/*
 * call poll; it is interruptable from SIGINT and SIGTERM signals (signalfd)
 * It will poll for getch, timerfd, inotify, pipe, signalfd and bus events.
 */
wint_t main_poll(WINDOW *win) {;
    uint64_t t;
    wint_t c;
    int ret = wget_wch(win, &c);
    /*
     * if ret == ERR, it means we did not receive a getch event.
     * so it is useless to return.
     */
    while ((ret == ERR) && (!quit)) {
        fix_input_cursor_pos(); // if we're currently asking a question, move cursor to its correct position on ASK_LINE
        /*
        * resize event returns -EPERM error with poll (-1)
        * see here: http://keyvanfatehi.com/2011/08/02/Asynchronous-c-programs-an-event-loop-and-ncurses/.
        * plus, this is needed when a signal is caught (sigint/sigterm)
        */
        int r = poll(main_p, nfds, -1);
        if (r == -1) {
            ret = wget_wch(win, &c); // ungetch(KEY_RESIZE);
        } else {
            for (int i = 0; i < nfds && r > 0; i++) {
                if (main_p[i].revents & POLLIN) {
                    switch (i) {
                    case GETCH_IX:
                    /* we received an user input */
                        ret = wget_wch(win, &c);
                        break;
                    case TIMER_IX:
                    /* we received a timer expiration signal on timerfd */
                        read(main_p[i].fd, &t, 8);
                        timer_event();
                        break;
                    case INOTIFY_IX1:
                    case INOTIFY_IX2:
                    /* we received an event from inotify */
                        inotify_refresh(i - INOTIFY_IX1);
                        break;
                    case INFO_IX:
                    /* we received an event from pipe to print an info msg */
                        info_refresh(main_p[i].fd);
                        break;
                    case SIGNAL_IX:
                    /* we received a signal */
                        sig_handler(main_p[i].fd);
                        break;
#if ARCHIVE_VERSION_NUMBER >= 3002000
                    case ARCHIVE_IX:
                    /* archiver thread needs a pwd for a protected archive */
                        archiver_cb_func();
                        break;
#endif
#ifdef SYSTEMD_PRESENT
                    case DEVMON_IX:
                    /* we received a bus event */
                        devices_bus_process();
                        break;
#endif
                    }
                    r--;
                }
            }
        }
    }
    return c;
}

#if ARCHIVE_VERSION_NUMBER >= 3002000
static void archiver_cb_func(void) {
    uint64_t u = 1;
    
    // if something fails, reset passphrase string and write in eventfd only to unlock 
    // extractor thread
    if (eventfd_read(archive_cb_fd[0], &u) == -1 || quit) {
        memset(passphrase, 0, strlen(passphrase));
        eventfd_write(archive_cb_fd[1], u);
        return;
    }
    ask_user(_(pwd_archive), passphrase, sizeof(passphrase));
    eventfd_write(archive_cb_fd[1], u);
}
#endif

/*
 * if received an external SIGINT or SIGTERM,
 * just switch the quit flag to 1 and log a warn.
 */
static void sig_handler(int fd) {
    char str[50];
    struct signalfd_siginfo fdsi;
    ssize_t s;
    
    s = read(fd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo)) {
        ERROR("an error occurred while getting signalfd data.");
    } else {
        sprintf(str, "received signal %d. Leaving.", fdsi.ssi_signo);
        WARN(str);
    }
    quit = NORM_QUIT;
}

void timer_event(void) {
    wmove(info_win, SYSTEM_INFO_LINE, 1);
    wclrtoeol(info_win);
    timer_func();
    wrefresh(info_win);
}

/*
 * Reads from info_pipe the address of the struct previously
 * allocated on heap by print_info function(),
 * and prints it to info_win.
 * Then free all the resources.
 */
static void info_refresh(int fd) {
    struct info_msg *info;
    
    read(fd, &info, sizeof(struct info_msg *));
    info_print(info->msg, info->line);
    free(info->msg);
    free(info);
}

/*
 * thanks: http://stackoverflow.com/questions/13351172/inotify-file-in-c
 */
static void inotify_refresh(int win) {
    size_t len, i = 0;
    char buffer[BUF_LEN];
    
    len = read(ps[win].inot.fd, buffer, BUF_LEN);
    while (i < len) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
        if (ps[win].mode <= fast_browse_) {
            /* ignore events for hidden files if ps[win].show_hidden is false */
            if ((event->len) && ((event->name[0] != '.') || (ps[win].show_hidden))) {
                if ((event->mask & IN_CREATE) || (event->mask & IN_DELETE) || event->mask & IN_MOVE) {
                    save_old_pos(win);
                    tab_refresh(win);
                } else if (event->mask & IN_MODIFY || event->mask & IN_ATTRIB) {
                    if (ps[win].mywin.stat_active) {
                        memset(ps[win].mywin.tot_size, 0, strlen(ps[win].mywin.tot_size));
                        show_stat(ps[win].mywin.delta, dim - 2, win);
                        print_border_and_title(win);
                    }
                }
            }
        }
        i += EVENT_SIZE + event->len;
    }
}

/*
 * Refreshes win UI if win is not in special_mode
 * (searching, bookmarks or device mode)
 */
void tab_refresh(int win) {
     if (ps[win].mode <= fast_browse_) {
        generate_list(win);
        if (strlen(ps[win].old_file)) {
            move_cursor_to_file(0, ps[win].old_file, win);
            memset(ps[win].old_file, 0, strlen(ps[win].old_file));
        }
    }
}

/*
 * Used to refresh special_mode windows.
 */
void update_special_mode(int num,  char (*str)[PATH_MAX + 1], int mode) {
    for (int win = 0; win < cont; win++) {
        if (ps[win].mode == mode) {
            if (str) {
            /* Do not reset win if a device/bookmark has been added. Just print next line */
                int check = num - ps[win].number_of_files;
                ps[win].number_of_files = num;
                str_ptr[win] = str;
                if (check < 0) {
                    /* update all */
                    reset_win(win);
                } else {
                    /* only update latest */
                    list_everything(win, num - 1, 1);
                }
            } else {
            /* Only used in device_monitor: change mounted status event */
                list_everything(win, num, 1);
            }
        }
    }
}

/*
 * Used when switching to special_mode.
 */
void show_special_tab(int num, char (*str)[PATH_MAX + 1], const char *title, int mode) {
    ps[active].mode = mode;
    ps[active].number_of_files = num;
    if (str != NULL) {
        str_ptr[active] = str;
        strncpy(ps[active].title, title, PATH_MAX);
        save_old_pos(active);
        print_additional_wins(HELPER_HEIGHT[normal], 0);
        reset_win(active);
    } else {
        // we're entering fast browse mode. We don't need to clear anything.
        // if we're entering special mode, we were in normal mode
        if (!print_additional_wins(HELPER_HEIGHT[normal], 0)) {
            // if no additional wins are present,
            // force a borders and arrow redraw
            print_border_and_title(active);
            print_arrow(active);
        }
    }
}

/*
 * used when leaving special mode.
 */
void leave_special_mode(const char *str,int win) {
    int old_mode = ps[win].mode;
    
    ps[win].mode = normal;
    if (str) {
        change_dir(str, win);
    }
    if (win == active) {
        // if we were in fast browse mode and there where no additional wins
        // force a borders and arrow redraw
        if (!print_additional_wins(HELPER_HEIGHT[old_mode], 0) && old_mode == fast_browse_) {
            print_border_and_title(active);
            print_arrow(active);
        }
    }
}

/*
 * Removes info win;
 * resizes every fm win, and moves it in the new position.
 * If helper_win != NULL, resizes it too, and moves it in the new position.
 * Fixes new current_position of every fm,
 * Then recreates info win.
 */
void resize_win(void) {
    wclear(info_win);
    delwin(info_win);
    info_win_init();
    print_additional_wins(HELPER_HEIGHT[ps[active].mode], 1);
    resize_fm_win();
}

static int print_additional_wins(int helper_height, int resizing) {
    int helper_win_needed = 0, fullname_win_needed = 0;
    
    dim = LINES - INFO_HEIGHT;
    if (helper_win) {
        dim -= helper_height;
    }
    if (fullname_win) {
        dim -= fullname_win_height;
        remove_additional_win(fullname_win_height, &fullname_win, resizing);
        fullname_win_needed = 1;
    }
    if (helper_win) {
        remove_additional_win(helper_height, &helper_win, resizing);
        helper_win_needed = 1;
    }
    if (helper_win_needed) {
        trigger_show_additional_win(HELPER_HEIGHT[ps[active].mode], &helper_win, helper_print);
    }
    if (fullname_win_needed) {
        trigger_fullname_win();
    }
    return helper_win_needed || fullname_win_needed;
}

/*
 * Clear every fm win, resize it and move it to its new position.
 * Fix new win's delta with new available LINES (ie: dim).
 * Then list_everything, being careful if stat_active is ON.
 */
static void resize_fm_win(void) {
    for (int i = 0; i < cont; i++) {
      resize_tab(i, 1);
    }
}

void change_sort(void) {
    ps[active].sorting_index = (ps[active].sorting_index + 1) % NUM(sorting_func);
    print_info(_(sorting_str[ps[active].sorting_index]), INFO_LINE);
    save_old_pos(active);
    tab_refresh(active);
}

/*
 * Called in manage_space_press() (fm_functions.c)
 * It checks for each fm win, if they're on the same cwd, then checks
 * if the file to be highlighted is visible inside "win" -> useful only if win is not the active one.
 * Then prints c char before current filename and refreshes win.
 */
void highlight_selected(const char *str, const char c, int win) {
    if (ps[win].mode <= fast_browse_) {
        int line = is_present(str, ps[win].nl, ps[win].number_of_files, -1, 0);
        if (line != -1 && (line - ps[win].mywin.delta >= 0) && (line - ps[win].mywin.delta < dim - 2)) {
            wattron(ps[win].mywin.fm, A_BOLD);
            mvwprintw(ps[win].mywin.fm, 1 + line - ps[win].mywin.delta, SEL_COL, "%c", c);
            wattroff(ps[win].mywin.fm, A_BOLD);
            wrefresh(ps[win].mywin.fm);
        }
    }
}

static void check_selected(const char *str, int win, int line) {
    int i = is_present(str, selected, num_selected, -1, 0);
    if (i != -1) {
        mvwprintw(ps[win].mywin.fm, 1 + line - ps[win].mywin.delta, SEL_COL, "*");
    }
}

void erase_selected_highlight(void) {
    for (int j = 0; j < cont; j++) {
        for (int i = 0; (i < dim - 2) && (i < ps[j].number_of_files); i++) {
            mvwprintw(ps[j].mywin.fm, 1 + i, SEL_COL, " ");
        }
        wrefresh(ps[j].mywin.fm);
    }
}

void update_colors(void) {
    print_arrow(!active);
    print_arrow(active);
    print_additional_wins(HELPER_HEIGHT[ps[!active].mode], 0); // switch helper_win context while changing tab
    print_border_and_title(!active);
    print_border_and_title(active);
}

void update_time(int where) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char date[70];
    
    strftime(date, sizeof(date), "%a %e %b %Y %k:%M", &tm);
    int x = check_sysinfo_where(where, strlen(date));
    if (x != -1) {
        mvwprintw(info_win, SYSTEM_INFO_LINE, x, "%.*s", COLS, date);
    }
}

void update_sysinfo(int where) {
    const long minute = 60;
    const long hour = minute * 60;
    const long day = hour * 24;
    const double megabyte = 1024 * 1024;
    char sys_str[100];
    int len;
    struct sysinfo si;
    
    sysinfo(&si);
    sprintf(sys_str, "up: %ldd, %ldh, %02ldm, ", si.uptime / day, (si.uptime % day) / hour, 
                                                    (si.uptime % hour) / minute);
    len = strlen(sys_str);
    sprintf(sys_str + len, "loads: %.2f, %.2f, %.2f, ", si.loads[0] / (float)(1 << SI_LOAD_SHIFT),
                                                        si.loads[1] / (float)(1 << SI_LOAD_SHIFT),
                                                        si.loads[2] / (float)(1 << SI_LOAD_SHIFT));
    float used_ram = (si.totalram - si.freeram) / megabyte;
    len = strlen(sys_str);
    sprintf(sys_str + len, "procs: %d, ram usage: %.1fMb/%.1fMb", si.procs, used_ram, si.totalram / megabyte);
    len = strlen(sys_str);
    int x = check_sysinfo_where(where, len);
    if (x != -1) {
        mvwprintw(info_win, SYSTEM_INFO_LINE, x, "%.*s", COLS, sys_str);
    }
}

void update_batt(int online, int perc[], int num_of_batt, char name[][10], int where) {
    char batt_str[num_of_batt][20];
    int len = 0;
    int x;
    
    switch (online) {
    case -1:
        /* built without libudev support. No info available. */
        x = check_sysinfo_where(where, strlen(_(power_fail)));
        if (x != -1) {
            mvwprintw(info_win, SYSTEM_INFO_LINE, x, "%.*s", COLS, _(power_fail));
        }
        break;
    case 1:
        /* ac connected */
        x = check_sysinfo_where(where, strlen(_(ac_online)));
        if (x != -1) {
            mvwprintw(info_win, SYSTEM_INFO_LINE, x, "%.*s", COLS, _(ac_online));
        }
        break;
    case 0:
        /* on battery */
        // to workaround an issue if sysinfo layout set
        // battery mon on center, and we have more than one battery.
        // we must check total batteries strings length before, to print it centered.
        for (int i = 0; i < num_of_batt; i++) {
            sprintf(batt_str[i], "%s: ", name[i]);
            if (perc[i] != -1) {
                sprintf(batt_str[i] + strlen(batt_str[i]), "%d%%%%", perc[i]);
                len += strlen(batt_str[i]) - 2;     /* -2 to delete spaces derived from %%%% */
            }
            len++;  /* if there's another bat, at least divide the two batteries by 1 space */
        }
        x = check_sysinfo_where(where, len);
        if (x != -1) {
            for (int i = 0; i < num_of_batt; i++) {
                if (perc[i] != -1) {
                    if (perc[i] <= config.bat_low_level) {
                        wattron(info_win, COLOR_PAIR(5));
                    }
                    mvwprintw(info_win, SYSTEM_INFO_LINE, x, batt_str[i]);
                    wattroff(info_win, COLOR_PAIR);
                }
            }
        }
        break;
    }
}

static int check_sysinfo_where(int where, int len) {
    switch (where) {
    case 0:
        return 1;
    case 1:
        return (COLS - len) / 2;
    case 2:
        return COLS - len;
    }
    return -1;
}

void trigger_fullname_win(void) {
    int len = strlen(str_ptr[active][ps[active].curr_pos]);
    fullname_win_height = len / COLS + 1;
    trigger_show_additional_win(fullname_win_height, &fullname_win, fullname_print);
}

static void fullname_print(void) {
    wattron(fullname_win, A_BOLD);
    colored_folders(fullname_win, str_ptr[active][ps[active].curr_pos]);
    mvwprintw(fullname_win, 0, 0, str_ptr[active][ps[active].curr_pos]);
    wattroff(fullname_win, COLOR_PAIR);
}

static void update_fullname_win(void) {
    remove_additional_win(fullname_win_height, &fullname_win, 0);
    trigger_fullname_win();
}

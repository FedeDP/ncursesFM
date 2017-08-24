/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * NcursesFM: file manager in C with ncurses UI for linux.
 * https://github.com/FedeDP/ncursesFM
 *
 * Copyright (C) 2016  Federico Di Pierro <nierro92@gmail.com>
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

#include "../inc/bookmarks.h"
#include "../inc/config.h"

static void locale_init(void);
static int set_signals(void);
static void set_pollfd(void);
static void sigsegv_handler(int signum);
static void check_desktop(void);
static void helper_function(int argc, char * const argv[]);
static void main_loop(void);
static void add_new_tab(void);
#ifdef SYSTEMD_PRESENT
static void check_device_mode(void);
#endif
static void manage_enter(struct stat current_file_stat);
static void manage_enter_search(struct stat current_file_stat);
static void manage_space(const char *str);
static void manage_quit(void);
static void switch_search(void);
static void check_remove(void (*f)(void));
static int check_init(int index);
static int check_access(void);
static void go_root_dir(void);

/*
 * pointers to long_file_operations functions, used in main loop;
 */
static int (*const long_func[LONG_FILE_OPERATIONS])(void) = {
    move_file, paste_file, remove_file, create_archive, extract_file
};

int main(int argc, char * const argv[])
{
    locale_init();
    helper_function(argc, argv);
#ifdef LIBCONFIG_PRESENT
    load_config_files();
#endif
    parse_cmd(argc, argv);
    open_log();
    config_checks();
    get_bookmarks();
    set_pollfd();
    init_job_queue();
#ifdef LIBNOTIFY_PRESENT
    init_notify();
#endif
    if (!quit) {
        screen_init();
        main_loop();
    }
    program_quit();
}

static void locale_init(void) {
    setlocale(LC_ALL, "");
    if (strcmp(nl_langinfo(CODESET), "UTF-8")) {
        fprintf(stderr, "Please use an utf8 locale.\n");
    }
    bindtextdomain("ncursesFM", LOCALEDIR);
    textdomain("ncursesFM");
}

static int set_signals(void) {
    sigset_t mask;
    
    // when receiving segfault signal,
    // call our sigsegv handler that just logs
    // a debug message before dying
    signal(SIGSEGV, sigsegv_handler);
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    return signalfd(-1, &mask, 0);
}

static void set_pollfd(void) {
#if ARCHIVE_VERSION_NUMBER >= 3002000
    nfds = 7;
#else
    nfds = 6;
#endif
#ifdef SYSTEMD_PRESENT
    nfds++;
#endif
    
    main_p = malloc(nfds * sizeof(struct pollfd));
    main_p[GETCH_IX] = (struct pollfd) {
        .fd = STDIN_FILENO,
        .events = POLLIN,
    };
    
    // do not start timer if no sysinfo info is enabled
    if (strlen(config.sysinfo_layout)) {
        main_p[TIMER_IX] = (struct pollfd) {
            .fd = start_timer(),
            .events = POLLIN,
        };
    }
    
    // inotify watcher init for each tab
    ps[0].inot.fd = inotify_init();
    ps[1].inot.fd = inotify_init();
    main_p[INOTIFY_IX1] = (struct pollfd) {
        .fd = ps[0].inot.fd,
        .events = POLLIN,
    };
    main_p[INOTIFY_IX2] = (struct pollfd) {
        .fd = ps[1].inot.fd,
        .events = POLLIN,
    };
    
    // info init. This is needed to let
    // multiple threads print an information string
    // without any issue.
    pipe(info_fd);
    main_p[INFO_IX] = (struct pollfd) {
        .fd = info_fd[0],
        .events = POLLIN,
    };
    
    // signalfd init to catch external signals
    main_p[SIGNAL_IX] = (struct pollfd) {
        .fd = set_signals(),
        .events = POLLIN,
    };
    
#if ARCHIVE_VERSION_NUMBER >= 3002000
    // NONBLOCK needed for EXTRACTOR_TH workaround when blocked 
    // inside a eventf read -> archive_cb_fd[0] is fd read by main_poll
    // this way it will return -1 if no data is waiting to be read.
    archive_cb_fd[0] = eventfd(0, EFD_NONBLOCK);
    archive_cb_fd[1] = eventfd(0, 0);
    main_p[ARCHIVE_IX] = (struct pollfd) {
        .fd = archive_cb_fd[0],
        .events = POLLIN,
    };
#endif
    
#ifdef SYSTEMD_PRESENT
    // device monitor watcher
    main_p[DEVMON_IX] = (struct pollfd) {
        .fd = start_monitor(),
        .events = POLLIN,
    };
#endif
}

/*
 * If received a sigsegv, only log a message then
 * set sigsegv signal handler to default (SIG_DFL),
 * and send again the signal to the process.
 */
static void sigsegv_handler(int signum) {
    ERROR("received sigsegv signal. Aborting.");
    close_log();
    signal(signum, SIG_DFL);
    kill(getpid(), signum);
}

/*
 * Check if ncursesFM is run by desktop environment (X or wayland)
 * or from a getty
 */
static void check_desktop(void) {
    has_desktop = getenv("XDG_SESSION_TYPE") != NULL;
}

static void helper_function(int argc, char * const argv[]) {
    char ncursesfm[150];
    sprintf(ncursesfm, "NcursesFM, version: %s, commit: %s, build time: %s.", VERSION, build_git_sha, build_git_time);
    if ((argc > 1) && (!strcmp(argv[1], "--help"))) {
        printf("\n NcursesFM\n");
        printf(" Version: %s\n", VERSION);
        printf(" Build time: %s\n", build_git_time);
        printf(" Commit: %s\n", build_git_sha);
        printf("\n Copyright (C) 2016  Federico Di Pierro (https://github.com/FedeDP):\n");
        printf(" This program comes with ABSOLUTELY NO WARRANTY;\n");
        printf(" This is free software, and you are welcome to redistribute it under certain conditions;\n");
        printf(" It is GPL licensed. Have a look at COPYING file.\n\n");
        printf("\tIt supports following cmdline options (they will override conf file settings):\n");
        printf("\t* --editor /path/to/editor to set an editor for current session. Fallbacks to $EDITOR env var.\n");
        printf("\t* --starting_dir /path/to/dir to set a starting directory for current session. Defaults to current dir.\n");
        printf("\t* --helper_win {0,1} to switch (off,on) starting helper message. Defaults to 1.\n");
#ifdef SYSTEMD_PRESENT
        printf("\t* --inhibit {0,1} to switch {off,on} powermanagement functions while a job is being processed. Defaults to 0.\n");
        printf("\t* --automount {0,1} to switch {off,on} automounting of external drives/usb sticks. Defaults to 0.\n");
#endif
        printf("\t* --loglevel {0,1,2,3} to change loglevel. Defaults to 0.\n");
        printf("\t\t* 0 to log only errors.\n\t\t* 1 to log warn messages and errors.\n");
        printf("\t\t* 2 to log info messages too.\n\t\t* 3 to disable logs.\n");
        printf("\t* --persistent_log {0,1} to switch {off,on} persistent logs across program restarts. Defaults to 0.\n");
        printf("\t* --low_level {$level} to set low battery signal's threshold. Defaults to 15%%.\n");
        printf("\t* --safe {0,1,2} to change safety level. Defaults to 2.\n");
        printf("\t\t* 0 don't ask ay confirmation.\n");
        printf("\t\t* 1 ask confirmation for file remotions/packages installs/printing files.\n");
        printf("\t\t* 2 ask confirmation for every action.\n\n");
        printf(" Have a look at /etc/default/ncursesFM.conf to set your global defaults.\n");
        printf(" You can copy default conf file to $HOME/.config/ncursesFM.conf to set your user defaults.\n");
        printf(" Just use arrow keys to move up and down, and enter to change directory or open a file.\n");
        printf(" Press 'L' while in program to view a more detailed helper message.\n\n");
        exit(EXIT_SUCCESS);
    }
    
    /* some default values */
    config.starting_helper = 1;
    config.bat_low_level = 15;
    config.safe = FULL_SAFE;
#ifdef SYSTEMD_PRESENT
    device_init = DEVMON_STARTING;
#endif
    wcscpy(config.cursor_chars, L"->");
    /* 
     * default sysinfo layout:
     * Clock
     * Process monitor
     * Battery monitor
     */
    strcpy(config.sysinfo_layout, "CPB");
    check_desktop();
}

/*
 * When in fast_browse_mode do not enter switch case;
 * if device_mode or search_mode are active on current window,
 * only 'q', 'l', or 't' (and enter, that is not printable char) can be called.
 * else stat current file and enter switch case.
 */
static void main_loop(void) {
    int index;
    char *ptr;
    
    /*
     * x to move,
     * v to paste,
     * r to remove,
     * b to compress,
     * z to extract
     */
    const char long_table[] = "xvrbz";

    /*
     * n, d to create new file/dir
     * o to rename.
     */
    const char short_table[] = "ndo";

    /*
     * l switch helper_win,
     * t new tab,
     * m only in device_mode to {un}mount device,
     * r in bookmarks_mode/selected mode to remove file from bookmarks/selected.
     * s to show stat
     * i to trigger fullname win
     */
    const char special_mode_allowed_chars[] = "ltmrsi";
    
    /*
     * Not graphical wchars:
     * arrow KEYS, needed to move cursor.
     * KEY_RESIZE to catch resize signals.
     * PG_UP/DOWN to move straight to first/last file.
     * KEY_MOUSE to catch mouse events
     * 32 -> space to select files
     * 127, backspace to go up to root folder
     */
    wchar_t not_graph_wchars[12];
    swprintf(not_graph_wchars, 12, L"%lc%lc%lc%lc%lc%lc%lc%lc%lc%lc%lc",  KEY_UP, KEY_DOWN, KEY_RIGHT,
                                                                    KEY_LEFT, KEY_RESIZE, KEY_PPAGE,
                                                                    KEY_NPAGE, KEY_MOUSE, (unsigned long)32,
                                                                    (unsigned long)127, KEY_BACKSPACE);
    
    MEVENT event;
#if NCURSES_MOUSE_VERSION > 1
    mousemask(BUTTON1_RELEASED | BUTTON2_RELEASED | BUTTON3_RELEASED | BUTTON4_PRESSED | BUTTON5_PRESSED, NULL);
#else
    mousemask(BUTTON1_RELEASED | BUTTON2_RELEASED | BUTTON3_RELEASED, NULL);
#endif

    while (!quit) {
        wint_t c = main_poll(ps[active].mywin.fm);
        if ((ps[active].mode == fast_browse_) && iswgraph(c) && !wcschr(not_graph_wchars, c)) {
            fast_browse(c);
            continue;
        }
        c = tolower(c);
        if (ps[active].mode > fast_browse_ && (isprint(c) && !strchr(special_mode_allowed_chars, c))) {
            continue;
        }
        struct stat current_file_stat = {0};
        stat(str_ptr[active][ps[active].curr_pos], &current_file_stat);
        switch (c) {
        case KEY_UP:
            scroll_up(active, 1);
            break;
        case KEY_DOWN:
            scroll_down(active, 1);
            break;
        case KEY_RIGHT:
        case KEY_LEFT:
            if (cont == MAX_TABS) {
                change_tab();
            }
            break;
        case KEY_PPAGE:
            scroll_up(active, ps[active].curr_pos);
            break;
        case KEY_NPAGE:
            scroll_down(active, ps[active].number_of_files - ps[active].curr_pos);
            break;
        case 127: case KEY_BACKSPACE: // backspace to go to root folder
            if (ps[active].mode <= fast_browse_) {
                go_root_dir();
            }
            break;
        case 'h': // h to show hidden files
            switch_hidden();
            break;
        case 10: // enter to change dir or open a file.
            manage_enter(current_file_stat);
            break;
        case 't': // t to open second tab
            if (cont < MAX_TABS) {
                add_new_tab();
                change_tab();
            }
            break;
        case 'w': // w to close second tab
            if (active) {
                cont--;
                delete_tab(1);
                resize_tab(0, 0);
                change_tab();
            }
            break;
        case 32: // space to select files
            manage_space(str_ptr[active][ps[active].curr_pos]);
            break;
        case 'l':  // show helper mess
            trigger_show_helper_message();
            break;
        case 's': // show stat about files (size and perms)
            trigger_stats();
            break;
        case 'e': // add file to bookmarks
            add_file_to_bookmarks(str_ptr[active][ps[active].curr_pos]);
            break;
        case 'f': // f to search
            switch_search();
            break;
#ifdef LIBCUPS_PRESENT
        case 'p': // p to print
            if ((S_ISREG(current_file_stat.st_mode)) && !(current_file_stat.st_mode & S_IXUSR)) {
                print_support(str_ptr[active][ps[active].curr_pos]);
            }
            break;
#endif
#ifdef SYSTEMD_PRESENT
        case 'm': // m to mount/unmount fs
            check_device_mode();
            break;
#endif
        case ',': // , to enable fast browse mode
            show_special_tab(ps[active].number_of_files, NULL, ps[active].title, fast_browse_);
            break;
        case 27: /* ESC to exit/leave special mode */
            manage_quit();
            break;
        case KEY_RESIZE:
            resize_win();
            break;
        case 9: // TAB to change sorting function
            if (ps[active].mode <= fast_browse_) {
                change_sort();
            }
            break;
        case 'i': // i to view current file fullname (in case it is too long)
            trigger_fullname_win();
            break;
        case 'n': case 'd': case 'o':   // fast operations do not require another thread.
            if (check_access()) {
                ptr = strchr(short_table, c);
                index = SHORT_FILE_OPERATIONS - strlen(ptr);
                fast_file_operations(index);
            }
            break;
        case 'g': // g to show bookmarks
            show_bookmarks();
            break;
        case 'k': // k to show selected files
            show_selected();
            break;
        case KEY_DC: // del to delete all selected files in selected mode/ all user bookmarks in bookmark mode
            if (ps[active].mode == bookmarks_) {
                check_remove(remove_all_user_bookmarks);
            } else if (ps[active].mode == selected_) {
                check_remove(remove_all_selected);
            }
            break;
        case KEY_MOUSE:
            if(getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_RELEASED) {
                    /* left click will send an enter event */
                    manage_enter(current_file_stat);
                } else if (event.bstate & BUTTON2_RELEASED) {
                    /* middle click will send a space event */
                    manage_space(str_ptr[active][ps[active].curr_pos]);
                } else if (event.bstate & BUTTON3_RELEASED) {
                    /* right click will send a back to root dir event */
                    if (ps[active].mode <= fast_browse_) {
                        go_root_dir();
                    }
                }
                /* scroll up and down events associated with mouse wheel */
#if NCURSES_MOUSE_VERSION > 1
                else if (event.bstate & BUTTON4_PRESSED) {
                    scroll_up(active, 1);
                } else if (event.bstate & BUTTON5_PRESSED) {
                    scroll_down(active, 1);
                }
#endif
            }
            break;
        default:
            ptr = strchr(long_table, c);
            if (ptr) {
                if (ps[active].mode == normal) {
                    index = LONG_FILE_OPERATIONS - strlen(ptr);
                    if (check_init(index)) {
                        init_thread(index, long_func[index]);
                    }
                // in mode != normal, only 'r' to remove is accepted while in bookmarks/selected mode
                } else if (ps[active].mode == bookmarks_) {
                    remove_bookmark_from_file();
                } else if (ps[active].mode == selected_) {
                    check_remove(remove_selected);
                }
            }
            break;
        }
    }
}

static void add_new_tab(void) {
    cont++;
    resize_tab(0, 0);
    new_tab(cont - 1);
}

#ifdef SYSTEMD_PRESENT
static void check_device_mode(void) {
    if (device_init == DEVMON_STARTING) {
        print_info(_(polling), INFO_LINE);
    } else if (device_init == DEVMON_READY && ps[active].mode == normal) {
        show_devices_tab();
    } else if (device_init == DEVMON_OFF) {
        print_info(_(monitor_err), INFO_LINE);
    } else if (ps[active].mode == device_) {
        manage_mount_device();
    }
}
#endif

static void manage_enter(struct stat current_file_stat) {
    if (ps[active].mode == search_) {
        manage_enter_search(current_file_stat);
    }
#ifdef SYSTEMD_PRESENT
    else if (ps[active].mode == device_) {
        manage_enter_device();
    }
#endif
    else if (ps[active].mode == bookmarks_) {
        manage_enter_bookmarks(current_file_stat);
    } else if (ps[active].mode == selected_) {
        leave_mode_helper(current_file_stat);
    } else if (S_ISDIR(current_file_stat.st_mode)) {
        change_dir(str_ptr[active][ps[active].curr_pos], active);
    } else {
        manage_file(str_ptr[active][ps[active].curr_pos]);
    }
}

static void manage_enter_search(struct stat current_file_stat) {
    char *str = NULL;
    char path[PATH_MAX + 1] = {0};
    
    strncpy(path, sv.found_searched[ps[active].curr_pos], PATH_MAX);
    if (!S_ISDIR(current_file_stat.st_mode)) {
        int index = search_enter_press(path);
        /* save in str current file's name */
        str = path + index + 1;
        /* check if this file was an archive and cut useless path inside archive */
        char *ptr = strchr(str, '/');
        if (ptr) {
            str[strlen(str) - strlen(ptr)] = '\0';
        }
        strncpy(ps[active].old_file, str, NAME_MAX);
        path[index] = '\0';
    } else {
        memset(ps[active].old_file, 0, strlen(ps[active].old_file));
    }
    leave_search_mode(path);
}

static void manage_space(const char *str) {
    if (ps[active].mode > fast_browse_) {
        return;
    }
    
    int all = !strcmp(strrchr(str, '/') + 1, "..");
    
    if (all) {
        manage_all_space_press();
    } else {
        manage_space_press(str);
    }
}

static void manage_quit(void) {
    if (ps[active].mode == search_) {
        leave_search_mode(ps[active].my_cwd);
    } else if (ps[active].mode > fast_browse_) {
        leave_special_mode(ps[active].my_cwd, active);
    } else if (ps[active].mode == fast_browse_) {
        leave_special_mode(NULL, active);
        print_info("", INFO_LINE); // clear fast browse string from info line
    } else {
        quit = NORM_QUIT;
    }
}

static void switch_search(void) {
    if (sv.searching == NO_SEARCH) {
        search();
    } else if (sv.searching == SEARCHING) {
        print_info(_(already_searching), INFO_LINE);
    } else if (sv.searching == SEARCHED) {
        list_found();
    }
}

static void check_remove(void (*f)(void)) {
    char c;
    ask_user(_(sure), &c, 1);
    if (c != _(yes)[0]) {
        return;
    }
    f();
}

static int check_init(int index) {
    char x;

    if (!selected) {
        print_info(_(no_selected_files), ERR_LINE);
        return 0;
    }
    if (index == EXTRACTOR_TH && config.safe == FULL_SAFE) {
        ask_user(_(extr_question), &x, 1);
        if (x == _(no)[0] || x == 27) {
            return 0;
        }
    }
    if (index != RM_TH) {
        return check_access();
    }
    if (config.safe != UNSAFE) {
        ask_user(_(sure), &x, 1);
        if (x != _(yes)[0]) {
            return 0;
        }
    }
    return 1;
}

static int check_access(void) {
    if (access(ps[active].my_cwd, W_OK) == -1) {
        print_info(strerror(errno), ERR_LINE);
        return 0;
    }
    return 1;
}

static void go_root_dir(void) {
    char root[PATH_MAX + 1] = {0};
        
    strncpy(root, ps[active].my_cwd, PATH_MAX);
    strcat(root, "/..");
    change_dir(root, active);
}

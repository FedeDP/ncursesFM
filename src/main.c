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

#ifdef LIBCONFIG_PRESENT
#include <libconfig.h>
#endif

static void set_signals(void);
static void set_pollfd(void);
static void sig_handler(int signum);
static void sigsegv_handler(int signum);
#ifdef LIBX11_PRESENT
static void check_X(void);
#endif
static void helper_function(int argc, const char *argv[]);
static void parse_cmd(int argc, const char *argv[]);
#ifdef LIBCONFIG_PRESENT
static void check_config_files();
static void read_config_file(const char *dir);
#endif
static void config_checks(void);
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
static int check_init(int index);
static int check_access(void);

/*
 * pointers to long_file_operations functions, used in main loop;
 */
static int (*const long_func[LONG_FILE_OPERATIONS])(void) = {
    move_file, paste_file, remove_file, create_archive, extract_file
};
int previewer_available, previewer_script_available;

int main(int argc, const char *argv[])
{
    set_signals();
    helper_function(argc, argv);
#ifdef LIBCONFIG_PRESENT
    check_config_files();
#endif
    parse_cmd(argc, argv);
    open_log();
    config_checks();
    get_bookmarks();
    set_pollfd();
    if (!quit) {
        screen_init();
        if (!quit) {
            main_loop();
        }
    }
    program_quit();
}

static void set_signals(void) {
    struct sigaction main_act = {{0}};
    sigset_t mask;

    main_act.sa_handler = sig_handler;
    sigaction(SIGINT, &main_act, 0);
    sigaction(SIGTERM, &main_act, 0);
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, &main_mask);
    signal(SIGSEGV, sigsegv_handler);
}

static void set_pollfd(void) {
#ifdef SYSTEMD_PRESENT
    nfds = 6;
#else
    nfds = 5;
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
    pipe(info_fd);
    main_p[INFO_IX] = (struct pollfd) {
        .fd = info_fd[0],
        .events = POLLIN,
    };
#ifdef SYSTEMD_PRESENT
    main_p[DEVMON_IX] = (struct pollfd) {
        .fd = start_monitor(),
        .events = POLLIN,
    };
#endif
}

/*
 * if received an external SIGINT or SIGTERM,
 * just switch the quit flag to 1 and log a warn.
 */
static void sig_handler(int signum) {
    char str[50];

    sprintf(str, "received signal %d. Leaving.", signum);
    WARN(str);
    quit = NORM_QUIT;
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

#ifdef LIBX11_PRESENT
static void check_X(void) {
    Display* display = XOpenDisplay(NULL);

    if (display) {
        XCloseDisplay(display);
        has_X = 1;
    }
}
#endif

static void helper_function(int argc, const char *argv[]) {
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
        printf("\t* --inhibit {0,1} to switch {off,on} powermanagement functions while a job is being processed. Defaults to 0.\n");
        printf("\t* --automount {0,1} to switch {off,on} automounting of external drives/usb sticks. Defaults to 0.\n");
        printf("\t* --loglevel {0,1,2,3} to change loglevel. Defaults to 0.\n");
        printf("\t\t* 0 to log only errors.\n\t\t* 1 to log warn messages and errors.\n");
        printf("\t\t* 2 to log info messages too.\n\t\t* 3 to disable logs.\n");
        printf("\t* --persistent_log {0,1} to switch {off,on} persistent logs across program restarts. Defaults to 0.\n");
        printf("\t* --low_level {$level} to set low battery signal's threshold. Defaults to 15%%.\n\n");
        printf(" Have a look at /etc/default/ncursesFM.conf to set your preferred defaults.\n");
        printf(" Just use arrow keys to move up and down, and enter to change directory or open a file.\n");
        printf(" Press 'L' while in program to view a more detailed helper message.\n\n");
        exit(EXIT_SUCCESS);
    }
    
    /* some default values */
    realpath(BINDIR, preview_bin);
    sprintf(preview_bin + strlen(preview_bin), "/ncursesfm_previewer");
    previewer_script_available = !access(preview_bin, X_OK);
    previewer_available = !access("/usr/lib/w3m/w3mimgdisplay", X_OK);
    config.starting_helper = 1;
    config.bat_low_level = 15;
#ifdef SYSTEMD_PRESENT
    device_init = DEVMON_STARTING;
#endif
    strcpy(config.border_chars, "||--++++");
    strcpy(config.cursor_chars, "->");
    /* 
     * default sysinfo layout: 
     * Clock
     * Process monitor
     * Battery monitor
     */
    strcpy(config.sysinfo_layout, "CPB");
#ifdef LIBX11_PRESENT
    check_X();
#endif
}

static void parse_cmd(int argc, const char *argv[]) {
    int j = 1;
#ifdef SYSTEMD_PRESENT
    const char *cmd_switch[] = {"--editor", "--starting_dir", "--helper_win", "--loglevel", "--persistent_log", "--low_level", "--inhibit", "--automount"};
#else
    const char *cmd_switch[] = {"--editor", "--starting_dir", "--helper_win", "--loglevel", "--persistent_log", "--low_level"};
#endif

    while (j < argc) {
        if ((!strcmp(cmd_switch[0], argv[j])) && (argv[j + 1])) {
            strcpy(config.editor, argv[j + 1]);
        } else if ((!strcmp(cmd_switch[1], argv[j])) && (argv[j + 1])) {
            strcpy(config.starting_dir, argv[j + 1]);
        } else if ((!strcmp(cmd_switch[2], argv[j])) && (argv[j + 1])) {
            config.starting_helper = atoi(argv[j + 1]);
        } else if ((!strcmp(cmd_switch[3], argv[j])) && (argv[j + 1])) {
            config.loglevel = atoi(argv[j + 1]);
        } else if ((!strcmp(cmd_switch[4], argv[j])) && (argv[j + 1])) {
            config.persistent_log = atoi(argv[j + 1]);
        } else if ((!strcmp(cmd_switch[5], argv[j])) && (argv[j + 1])) {
            config.bat_low_level = atoi(argv[j + 1]);
        }
#ifdef SYSTEMD_PRESENT
        else if ((!strcmp(cmd_switch[6], argv[j])) && (argv[j + 1])) {
            config.inhibit = atoi(argv[j + 1]);
        }
        else if ((!strcmp(cmd_switch[7], argv[j])) && (argv[j + 1])) {
            config.automount = atoi(argv[j + 1]);
        }
#endif
        else {
            break;
        }
        j += 2;
    }
    if (j != argc) {
        printf("Options not recognized. Use '--help' to view helper message.\n");
        exit(EXIT_FAILURE);
    }
}

#ifdef LIBCONFIG_PRESENT
static void check_config_files() {
    read_config_file(CONFDIR);
    
    char home_config_path[PATH_MAX + 1];
    
    sprintf(home_config_path, "%s/.config", getpwuid(getuid())->pw_dir);
    read_config_file(home_config_path);
}

static void read_config_file(const char *dir) {
    config_t cfg;
    char config_file_name[PATH_MAX + 1];
    const char *str_editor, *str_starting_dir, 
                *str_borders, *str_cursor, *sysinfo;

    sprintf(config_file_name, "%s/ncursesFM.conf", dir);
    if (access(config_file_name, F_OK ) == -1) {
        fprintf(stderr, "Config file %s not found.\n", config_file_name);
        return;
    }
    config_init(&cfg);
    if (config_read_file(&cfg, config_file_name) == CONFIG_TRUE) {
        if (config_lookup_string(&cfg, "editor", &str_editor) == CONFIG_TRUE) {
            strcpy(config.editor, str_editor);
        }
        config_lookup_int(&cfg, "show_hidden", &config.show_hidden);
        if (config_lookup_string(&cfg, "starting_directory", &str_starting_dir) == CONFIG_TRUE) {
            strcpy(config.starting_dir, str_starting_dir);
        }
        config_lookup_int(&cfg, "use_default_starting_dir_second_tab", &config.second_tab_starting_dir);
        config_lookup_int(&cfg, "starting_helper", &config.starting_helper);
#ifdef SYSTEMD_PRESENT
        config_lookup_int(&cfg, "inhibit", &config.inhibit);
        config_lookup_int(&cfg, "automount", &config.automount);
#endif
        config_lookup_int(&cfg, "loglevel", &config.loglevel);
        config_lookup_int(&cfg, "persistent_log", &config.persistent_log);
        config_lookup_int(&cfg, "bat_low_level", &config.bat_low_level);
        if (config_lookup_string(&cfg, "border_chars", &str_borders) == CONFIG_TRUE) {
            strncpy(config.border_chars, str_borders, sizeof(config.border_chars));
        }
        if (config_lookup_string(&cfg, "cursor_chars", &str_cursor) == CONFIG_TRUE) {
            strncpy(config.cursor_chars, str_cursor, sizeof(config.cursor_chars));
        }
        if (config_lookup_string(&cfg, "sysinfo_layout", &sysinfo) == CONFIG_TRUE) {
            strncpy(config.sysinfo_layout, sysinfo, sizeof(config.sysinfo_layout));
        }
    } else {
        fprintf(stderr, "Config file: %s at line %d.\n",
                config_error_text(&cfg),
                config_error_line(&cfg));
    }
    config_destroy(&cfg);
}
#endif

static void config_checks(void) {
    const char *str;

    if ((strlen(config.starting_dir)) && (access(config.starting_dir, F_OK) == -1)) {
        memset(config.starting_dir, 0, strlen(config.starting_dir));
    }
    if (!strlen(config.editor) || (access(config.editor, X_OK) == -1)) {
        memset(config.editor, 0, strlen(config.editor));
        WARN("no editor defined. Trying to get one from env.");
        if ((str = getenv("EDITOR"))) {
            strcpy(config.editor, str);
        } else {
            WARN("no editor env var found.");
        }
    }
    if ((config.loglevel < LOG_ERR) || (config.loglevel > NO_LOG)) {
        config.loglevel = LOG_ERR;
    }
}

/*
 * When in fast_browse_mode do not enter switch case;
 * if device_mode or search_mode are active on current window,
 * only 'q', 'l', or 't' (and enter, that is not printable char) can be called.
 * else stat current file and enter switch case.
 */
static void main_loop(void) {
    int c, index;

    /*
     * x to move,
     * v to paste,
     * r to remove,
     * b to compress,
     * z to extract
     */
    const char *long_table = "xvrbz";

    /*
     * n, d to create new file/dir
     * o to rename.
     */
    const char *short_table = "ndo";

    /*
     * l switch helper_win,
     * t new tab,
     * m only in device_mode to {un}mount device,
     * e only in bookmarks_mode to remove device from bookmarks
     * s to show stat
     * i to trigger fullname win
     */
    const char *special_mode_allowed_chars = "ltmesi";

    char *ptr, old_file[PATH_MAX + 1] = "";
    struct stat current_file_stat;
    
    MEVENT event;
#if NCURSES_MOUSE_VERSION > 1
    mousemask(BUTTON1_RELEASED | BUTTON2_RELEASED | BUTTON3_RELEASED | BUTTON4_PRESSED | BUTTON5_PRESSED, NULL);
#else
    mousemask(BUTTON1_RELEASED | BUTTON2_RELEASED | BUTTON3_RELEASED, NULL);
#endif

    while (!quit) {
        if (ps[1].mode == _preview && strcmp(old_file, str_ptr[active][ps[active].curr_pos])) {
            preview_img(str_ptr[active][ps[active].curr_pos]);
            strcpy(old_file, str_ptr[active][ps[active].curr_pos]);
        }
        c = main_poll(ps[active].mywin.fm);
        if ((ps[active].mode == fast_browse_) && isgraph(c) && (c != ',')) {
            fast_browse(c);
            continue;
        }
        c = tolower(c);
        if (ps[active].mode > fast_browse_ && isprint(c) && !strchr(special_mode_allowed_chars, c)) {
            continue;
        }
        stat(str_ptr[active][ps[active].curr_pos], &current_file_stat);
        switch (c) {
        case KEY_UP:
            scroll_up(active);
            break;
        case KEY_DOWN:
            scroll_down(active);
            break;
        case KEY_RIGHT:
        case KEY_LEFT:
            if (cont == MAX_TABS && ps[1].mode != _preview) {
                change_tab();
            }
            break;
        case KEY_PPAGE:
            if (ps[active].curr_pos > 0) {
                ptr = strrchr(ps[active].nl[0], '/') + 1;
                move_cursor_to_file(0, ptr, active);
            }
            break;
        case KEY_NPAGE:
            if (ps[active].curr_pos < ps[active].number_of_files - 1) {
                ptr = strrchr(ps[active].nl[ps[active].number_of_files - 1], '/') + 1;
                move_cursor_to_file(ps[active].number_of_files - 1, ptr, active);
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
        case 'e': // add dir to bookmarks
            if (ps[active].mode <= fast_browse_) {
                add_file_to_bookmarks(str_ptr[active][ps[active].curr_pos]);
            } else if (ps[active].mode == bookmarks_) {
                remove_bookmark_from_file();
            }
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
        case ',': // , to enable/disable fast browse mode
            switch_fast_browse_mode();
            break;
        case 27: /* ESC to exit/leave special mode */
            manage_quit();
            break;
        case KEY_RESIZE:
            resize_win();
            break;
        case '.': // . to change sorting function
            change_sort();
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
        case 'j': // j to trigger image previewer
            if (cont == 1 && previewer_available && has_X && previewer_script_available) {
                ps[1].mode = _preview;
                add_new_tab();
            } else if (ps[1].mode == _preview) {
                cont--;
                delete_tab(1);
                resize_tab(0, 0);
                memset(old_file, 0, strlen(old_file));
            } else if (!previewer_available) {
                print_info("You need w3mimgdisplay from w3m to preview images.", INFO_LINE);
            } else if (!previewer_script_available) {
                print_info( "Previewing script not found.", ERR_LINE);
            } else if (!has_X) {
                print_info("You need to be in a X session for image preview to work.", INFO_LINE);
            }
            break;
        case KEY_MOUSE:
            if(getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_RELEASED) {
                    /* left click will send an enter event */
                    manage_enter(current_file_stat);
                } else if (event.bstate & BUTTON2_RELEASED) {
                    /* middle click will send a "new tab" event */
                    if (cont < MAX_TABS) {
                        add_new_tab();
                        change_tab();
                    }
                } else if (event.bstate & BUTTON3_RELEASED) {
                    /* right click will send a space event */
                    manage_space(str_ptr[active][ps[active].curr_pos]);
                }
                /* scroll up and down events associated with mouse wheel */
#if NCURSES_MOUSE_VERSION > 1
                else if (event.bstate & BUTTON4_PRESSED) {
                    scroll_up(active);
                } else if (event.bstate & BUTTON5_PRESSED) {
                    scroll_down(active);
                }
#endif
            }
            break;
        default:
            ptr = strchr(long_table, c);
            if (ptr) {
                index = LONG_FILE_OPERATIONS - strlen(ptr);
                if (check_init(index)) {
                    init_thread(index, long_func[index]);
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
        print_info("Still polling for initial devices.", INFO_LINE);
    } else if (device_init == DEVMON_READY && ps[active].mode <= fast_browse_) {
        show_devices_tab();
    } else if (device_init == DEVMON_OFF) {
        print_info("Monitor is not active. An error occurred, check log file.", INFO_LINE);
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
    } else if (S_ISDIR(current_file_stat.st_mode)) {
        change_dir(str_ptr[active][ps[active].curr_pos], active);
    } else {
        manage_file(str_ptr[active][ps[active].curr_pos], current_file_stat.st_size);
    }
}

static void manage_enter_search(struct stat current_file_stat) {
    char *str = NULL;
    char path[PATH_MAX + 1];
    
    strcpy(path, sv.found_searched[ps[active].curr_pos]);
    if (!S_ISDIR(current_file_stat.st_mode)) {
        int index = search_enter_press(path);
        /* save in str current file's name */
        str = path + index + 1;
        /* check if this file was an archive and cut useless path inside archive */
        char *ptr = strchr(str, '/');
        if (ptr) {
            str[strlen(str) - strlen(ptr)] = '\0';
        }
        strcpy(ps[active].old_file, str);
        path[index] = '\0';
    } else {
        memset(ps[active].old_file, 0, strlen(ps[active].old_file));
    }
    leave_search_mode(path);
}

static void manage_space(const char *str) {
    if (strcmp(strrchr(str, '/') + 1, "..") && ps[active].mode <= fast_browse_) {
        manage_space_press(str);
    }
}

static void manage_quit(void) {
    if (ps[active].mode > fast_browse_) {
        if (ps[active].mode == search_) {
            leave_search_mode(ps[active].my_cwd);
        } else {
            leave_special_mode(ps[active].my_cwd);
        }
    } else {
        quit = NORM_QUIT;
    }
}

static void switch_search(void) {
    if (sv.searching == NO_SEARCH) {
        search();
    } else if (sv.searching == SEARCHING) {
        print_info(already_searching, INFO_LINE);
    } else if (sv.searching == SEARCHED) {
        list_found();
    }
}

static int check_init(int index) {
    char x;

    if (!selected && index != EXTRACTOR_TH) {
        print_info(no_selected_files, ERR_LINE);
        return 0;
    }
    if (index == EXTRACTOR_TH) {
        ask_user(extr_question, &x, 1);
        if (quit || x == 'n' || x == 27) {
            return 0;
        }
    }
    if (index != RM_TH) {
        return check_access();
    }
    ask_user(sure, &x, 1);
    if (quit || x != 'y') {
        return 0;
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

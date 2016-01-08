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

#include "../inc/fm_functions.h"

#ifdef LIBCONFIG_PRESENT
#include <libconfig.h>
#endif

static void helper_function(int argc, const char *argv[]);
static void parse_cmd(int argc, const char *argv[]);
#ifdef LIBCONFIG_PRESENT
static void read_config_file(void);
#endif
static void config_checks(void);
static void main_loop(void);
static int check_init(int index);
static int check_access(void);
static void set_signals(void);
static void sig_handler(int signum);

/*
 * pointers to long_file_operations functions, used in main loop;
 * -1 because extract operation is called inside "enter press" event, not in main loop
 */
static int (*const long_func[LONG_FILE_OPERATIONS - 1])(void) = {
    move_file, paste_file, remove_file, create_archive
};

int main(int argc, const char *argv[])
{
    set_signals();
    helper_function(argc, argv);
#ifdef LIBCONFIG_PRESENT
    read_config_file();
#endif
    parse_cmd(argc, argv);
    open_log();
    config_checks();
#ifdef LIBUDEV_PRESENT
    start_monitor();
#endif
    screen_init();
    chdir(ps[active].my_cwd);
    main_loop();
    program_quit();
}

static void helper_function(int argc, const char *argv[]) {
    /* default value for starting_helper */
    config.starting_helper = 1;
#ifdef LIBUDEV_PRESENT
    device_mode = DEVMON_STARTING;
#else
    device_mode = DEVMON_OFF;
#endif
    if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
        printf("\n NcursesFM Copyright (C) 2016  Federico Di Pierro (https://github.com/FedeDP):\n");
        printf(" This program comes with ABSOLUTELY NO WARRANTY;\n");
        printf(" This is free software, and you are welcome to redistribute it under certain conditions;\n");
        printf(" It is GPL licensed. Have a look at COPYING file.\n\n");
        printf("\tIt supports following cmdline options (they will override conf file settings):\n");
        printf("\t* --editor /path/to/editor to set an editor for current session. Fallbacks to $EDITOR env var.\n");
        printf("\t* --starting_dir /path/to/dir to set a starting directory for current session. Defaults to current dir.\n");
        printf("\t* --helper {0,1} to switch (off,on) starting helper message. Defaults to 1.\n");
        printf("\t* --inhibit {0,1} to switch {off,on} powermanagement functions while a job is being processed. Defaults to 0.\n");
        printf("\t* --automount {0,1} to switch {off,on} automounting of external drives/usb sticks. Defaults to 0.\n");
        printf("\t* --loglevel {0,1,2,3} to change loglevel. Defaults to 0.\n");
        printf("\t\t* 0 to log only errors.\n\t\t* 1 to log warn messages and errors.\n");
        printf("\t\t* 2 to log info messages too.\n\t\t* 3 to disable log.\n");
        printf("\t* --persistent_log {0,1} to switch {off,on} persistent log across program restarts. Defaults to 0.\n\n");
        printf(" Have a look at /etc/default/ncursesFM.conf to set your preferred defaults.\n");
        printf(" Just use arrow keys to move up and down, and enter to change directory or open a file.\n");
        printf(" Press 'l' while in program to view a more detailed helper message.\n\n");
        exit(EXIT_SUCCESS);
    }
}

static void parse_cmd(int argc, const char *argv[]) {
    int j = 1;
#ifdef SYSTEMD_PRESENT
#ifdef LIBUDEV_PRESENT
    const char *cmd_switch[] = {"--editor", "--starting_dir", "--helper", "--loglevel", "--persistent_log", "--inhibit", "--automount"};
#else
    const char *cmd_switch[] = {"--editor", "--starting_dir", "--helper", "--loglevel", "--persistent_log", "--inhibit"};
#endif
#else
    const char *cmd_switch[] = {"--editor", "--starting_dir", "--helper", "--loglevel", "--persistent_log"};
#endif

    while (j < argc) {
        if ((strcmp(cmd_switch[0], argv[j]) == 0) && (argv[j + 1])) {
            strcpy(config.editor, argv[j + 1]);
        } else if ((strcmp(cmd_switch[1], argv[j]) == 0) && (argv[j + 1])) {
            strcpy(config.starting_dir, argv[j + 1]);
        } else if ((strcmp(cmd_switch[2], argv[j]) == 0) && (argv[j + 1])) {
            config.starting_helper = atoi(argv[j + 1]);
        } else if ((strcmp(cmd_switch[3], argv[j]) == 0) && (argv[j + 1])) {
            config.loglevel = atoi(argv[j + 1]);
        } else if ((strcmp(cmd_switch[4], argv[j]) == 0) && (argv[j + 1])) {
            config.persistent_log = atoi(argv[j + 1]);
        }
#ifdef SYSTEMD_PRESENT
        else if ((strcmp(cmd_switch[5], argv[j]) == 0) && (argv[j + 1])) {
            config.inhibit = atoi(argv[j + 1]);
        }
#ifdef LIBUDEV_PRESENT
        else if ((strcmp(cmd_switch[6], argv[j]) == 0) && (argv[j + 1])) {
            config.automount = atoi(argv[j + 1]);
        }
#endif
#endif
        else {
            break;
        }
        j += 2;
    }
    if (j != argc) {
        printf("Option not recognized. Use '--help' to view helper message.\n");
        exit(EXIT_FAILURE);
    }
}

#ifdef LIBCONFIG_PRESENT
static void read_config_file(void) {
    config_t cfg;
    const char *config_file_name = "/etc/default/ncursesFM.conf";
    const char *str_editor, *str_starting_dir;

    config_init(&cfg);
    if (config_read_file(&cfg, config_file_name) == CONFIG_TRUE) {
        if ((!strlen(config.editor)) && (config_lookup_string(&cfg, "editor", &str_editor) == CONFIG_TRUE)) {
            strcpy(config.editor, str_editor);
        }
        config_lookup_int(&cfg, "show_hidden", &config.show_hidden);
        if ((!strlen(config.starting_dir)) && (config_lookup_string(&cfg, "starting_directory", &str_starting_dir) == CONFIG_TRUE)) {
            strcpy(config.starting_dir, str_starting_dir);
        }
        config_lookup_int(&cfg, "use_default_starting_dir_second_tab", &config.second_tab_starting_dir);
        config_lookup_int(&cfg, "starting_helper", &config.starting_helper);
#ifdef SYSTEMD_PRESENT
        config_lookup_int(&cfg, "inhibit", &config.inhibit);
#ifdef LIBUDEV_PRESENT
        config_lookup_int(&cfg, "automount", &config.automount);
#endif
#endif
        config_lookup_int(&cfg, "loglevel", &config.loglevel);
        config_lookup_int(&cfg, "persistent_log", &config.persistent_log);
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
        WARN("wrong loglevel value. Back to default value.");
    }
}

/*
 * When in fast_browse_mode do not enter switch case;
 * if device_mode or search_mode are active on current window,
 * only 'q', 'l', or 't' (and enter, that is not printable char) can be called.
 * else stat current file and enter switch case.
 */
static void main_loop(void) {
    int c, index, fast_browse_mode = 0;
    const char *long_table = "xvrb"; // x to move, v to paste, r to remove, b to compress
    const char *short_table = "ndo";  //n, d to create new file/dir, o to rename.
    const char *special_mode_allowed_chars = "ltq";
    char *ptr;
    struct stat current_file_stat;

    while (!quit) {
        c = win_getch();
        if ((fast_browse_mode == 1 + active) && isprint(c) && (c != ',')) {
            fast_browse(c);
            continue;
        }
        c = tolower(c);
        if ((device_mode == 1 + active) || (sv.searching == 3 + active)) {
            if (isprint(c) && (!strchr(special_mode_allowed_chars, c))) {
                continue;
            }
        } else {
            stat(ps[active].nl[ps[active].curr_pos], &current_file_stat);
        }
        switch (c) {
        case KEY_UP:
            scroll_up();
            break;
        case KEY_DOWN:
            scroll_down();
            break;
        case KEY_RIGHT:
            if (!active && cont == MAX_TABS) {
                change_tab();
            }
            break;
        case KEY_LEFT:
            if (active) {
                change_tab();
            }
            break;
        case 'h': // h to show hidden files
            switch_hidden();
            break;
        case 10: // enter to change dir or open a file.
            if (sv.searching == 3 + active) {
                index = search_enter_press(sv.found_searched[ps[active].curr_pos]);
                sv.found_searched[ps[active].curr_pos][index] = '\0';
                leave_search_mode(sv.found_searched[ps[active].curr_pos]);
            }
#ifdef LIBUDEV_PRESENT
            else if (device_mode == 1 + active) {
                manage_enter_device();
            }
#endif
            else if (S_ISDIR(current_file_stat.st_mode)) {
                change_dir(ps[active].nl[ps[active].curr_pos]);
            } else {
                manage_file(ps[active].nl[ps[active].curr_pos], current_file_stat.st_size);
            }
            break;
        case 't': // t to open second tab
            if (cont < MAX_TABS) {
                cont++;
                change_first_tab_size();
                new_tab(cont - 1);
                change_tab();
            }
            break;
        case 'w': // w to close second tab
            if (active) {
                change_tab();
                cont--;
                delete_tab(cont);
                change_first_tab_size();
            }
            break;
        case 32: // space to select files
            if (strcmp(strrchr(ps[active].nl[ps[active].curr_pos], '/') + 1, "..") != 0) {
                manage_space_press(ps[active].nl[ps[active].curr_pos]);
            }
            break;
        case 'l':  // show helper mess
            trigger_show_helper_message();
            break;
        case 's': // show stat about files (size and perms)
            trigger_stats();
            break;
        case 'f': // f to search
            if (sv.searching == 0) {
                search();
            } else if (sv.searching == 1) {
                print_info(already_searching, INFO_LINE);
            } else if (sv.searching == 2) {
                list_found();
            }
            break;
#ifdef LIBCUPS_PRESENT
        case 'p': // p to print
            if ((S_ISREG(current_file_stat.st_mode)) && !(current_file_stat.st_mode & S_IXUSR)) {
                print_support(ps[active].nl[ps[active].curr_pos]);
            }
            break;
#endif
#ifdef LIBUDEV_PRESENT
        case 'm': // m to mount/unmount fs
            if (device_mode == DEVMON_OFF) {
                print_info("Monitor is not active. An error occurred, check log file.", INFO_LINE);
            } else if (device_mode > DEVMON_READY) {
                print_info("A tab is already in device mode.", INFO_LINE);
            } else {
                show_devices_tab();
            }
            break;
#endif
        case ',': // , to enable/disable fast browse mode
            if (!fast_browse_mode) {
                fast_browse_mode = 1 + active;
                print_info("Fast browse mode enabled.", INFO_LINE);
            } else {
                fast_browse_mode = 0;
                print_info("Fast browse mode disabled.", INFO_LINE);
            }
            break;
#ifdef OPENSSL_PRESENT
        case 'u': // u to check current file's shasum
            if (strcmp(strrchr(ps[active].nl[ps[active].curr_pos], '/') + 1, "..") != 0) {
                shasum_func(ps[active].nl[ps[active].curr_pos]);
            }
            break;
#endif
        case 'q': /* q to exit/leave search mode/leave device_mode */
            if (sv.searching == 3 + active) {
                leave_search_mode(ps[active].my_cwd);
            }
#ifdef LIBUDEV_PRESENT
            else if (device_mode == 1 + active) {
                leave_device_mode();
            }
#endif
            else {
                quit = NORM_QUIT;
            }
            break;
        case KEY_RESIZE:
            resize_win();
            break;
        case '.': // . to change sorting function
            change_sort();
            break;
        case 'n': case 'd': case 'o':   // fast operations do not require another thread.
            if (check_access()) {
                ptr = strchr(short_table, c);
                index = SHORT_FILE_OPERATIONS - strlen(ptr);
                fast_file_operations(index);
            }
            break;
        default:
            ptr = strchr(long_table, c);
            if (ptr) {
                index = LONG_FILE_OPERATIONS - 1 - strlen(ptr);
                if (check_init(index)) {
                    init_thread(index, long_func[index]);
                }
            }
            break;
        }
    }
}

static int check_init(int index) {
    char x;

    if (!selected) {
        print_info(no_selected_files, ERR_LINE);
        return 0;
    }
    if (index != RM_TH) {
        return check_access();
    }
    ask_user(sure, &x, 1, 'n');
    return (x == 'y') ? 1 : 0;
}

static int check_access(void) {
    if (access(ps[active].my_cwd, W_OK) == -1) {
        print_info(strerror(errno), ERR_LINE);
        return 0;
    }
    return 1;
}

static void set_signals(void) {
    signal(SIGSEGV, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
}

static void sig_handler(int signum) {
    char str[100];
    
    sprintf(str, "received signal %d.", signum);
    ERROR(str);
    close_log();
    signal(signum, SIG_DFL);
    kill(getpid(), signum);
}
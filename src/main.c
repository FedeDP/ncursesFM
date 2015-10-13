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

#include "fm_functions.h"
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

/*
 * pointers to long_file_operations functions, used in main loop;
 * -1 because extract operation is called inside "enter press" event, not in main loop
 */
static int (*const long_func[LONG_FILE_OPERATIONS - 1])(void) = {
    move_file, paste_file, remove_file, create_archive
};

int main(int argc, const char *argv[])
{
    main_id = pthread_self();
    helper_function(argc, argv);
#ifdef LIBCONFIG_PRESENT
    read_config_file();
#endif
    config_checks();
    screen_init();
    change_tab();
    main_loop();
    free_everything();
    screen_end();
    printf("\033c"); // to clear terminal/vt after leaving program
    return 0;
}

static void helper_function(int argc, const char *argv[])
{
    if (argc != 1) {
        if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
            printf("\tNcursesFM Copyright (C) 2015  Federico Di Pierro (https://github.com/FedeDP):\n");
            printf("\tThis program comes with ABSOLUTELY NO WARRANTY;\n");
            printf("\tThis is free software, and you are welcome to redistribute it under certain conditions;\n");
            printf("\tIt is GPL licensed. Have a look at COPYING file.\n");
            printf("\t\t* -h/--help to view this helper message.\n");
            printf("\t\t* --editor=/path/to/editor to set an editor for current session.\n");
            printf("\t\t* --starting-dir=/path/to/dir to set a starting directory for current session.\n");
            printf("\t\t* --inhibit=1 to switch off powermanagement functions only while a joblist is being processed.\n");
            printf("\t\t* Have a look at the config file /etc/default/ncursesFM.conf to set your preferred defaults.\n");
            printf("\t\t* Just use arrow keys to move up and down, and enter to change directory or open a file.\n");
            printf("\t\t* Press 'l' while in program to view a more detailed helper message.\n");
            exit(0);
        } else {
            parse_cmd(argc, argv);
        }
    }
}

static void parse_cmd(int argc, const char *argv[])
{
    int j = 1, changed = 1;
    const char *cmd_switch[] = {"--editor=", "--starting-dir=", "--inhibit="};

    while (j < argc) {
        if (strncmp(cmd_switch[0], argv[j], strlen(cmd_switch[0])) == 0) {
            if (!strlen(config.editor)) {
                strcpy(config.editor, argv[j] + strlen(cmd_switch[0]));
                changed++;
            }
        } else if (strncmp(cmd_switch[1], argv[j], strlen(cmd_switch[1])) == 0) {
            if (!strlen(config.starting_dir)) {
                strcpy(config.starting_dir, argv[j] + strlen(cmd_switch[1]));
                changed++;
            }
        } else if (!config.inhibit && strncmp(cmd_switch[2], argv[j], strlen(cmd_switch[2])) == 0) {
            config.inhibit = atoi(argv[j] + strlen(cmd_switch[2]));
            changed++;
        }
        j++;
    }
    if (changed != argc) {
        printf("Use '-h' to view helper message.\n");
        exit(0);
    }
}

#ifdef LIBCONFIG_PRESENT
static void read_config_file(void)
{
    config_t cfg;
    const char *config_file_name = "/etc/default/ncursesFM.conf";
    const char *str_editor, *str_starting_dir;

    config_init(&cfg);
    if (config_read_file(&cfg, config_file_name)) {
        if ((!strlen(config.editor)) && (config_lookup_string(&cfg, "editor", &str_editor))) {
            strcpy(config.editor, str_editor);
        }
        config_lookup_int(&cfg, "show_hidden", &config.show_hidden);
        if ((!strlen(config.starting_dir)) && (config_lookup_string(&cfg, "starting_directory", &str_starting_dir))) {
            strcpy(config.starting_dir, str_starting_dir);
        }
        config_lookup_int(&cfg, "use_default_starting_dir_second_tab", &config.second_tab_starting_dir);
        config_lookup_int(&cfg, "inhibit", &config.inhibit);
    } else {
        printf("%s", config_file_missing);
        sleep(1);
    }
    config_destroy(&cfg);
}
#endif

static void config_checks(void)
{
    const char *str;

    if ((strlen(config.starting_dir)) && (access(config.starting_dir, F_OK) == -1)) {
        memset(config.starting_dir, 0, strlen(config.starting_dir));
    }
    if (!strlen(config.editor) || (access(config.editor, X_OK) == -1)) {
        memset(config.editor, 0, strlen(config.editor));
        if ((str = getenv("EDITOR"))) {
            strcpy(config.editor, str);
        }
    }
}

static void main_loop(void)
{
    int c, index, fast_browse_mode = 0, help = 0;
    const char *long_table = "xvrb"; // x to move, v to paste, r to remove, b to compress
    const char *short_table = "ndo";  //n, d to create new file/dir, o to rename.
    char *ptr;
    struct stat current_file_stat;

    while (!quit) {
        c = win_refresh_and_getch();
        if ((fast_browse_mode == 1 + active) && isprint(c) && (c != ',')) {
            fast_browse(c);
            continue;
        }
        c = tolower(c);
        if ((device_mode == 1 + active) && isprint(c) && (c != 'q') && (c != 'l')) {
            continue;
        }
        if ((sv.searching != 3 + active) && (device_mode != 1 + active)) {
            stat(ps[active].nl[ps[active].curr_pos], &current_file_stat);
        }
        switch (c) {
        case KEY_UP:
            scroll_up();
            break;
        case KEY_DOWN:
            scroll_down();
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
#if defined(LIBUDEV_PRESENT) && (SYSTEMD_PRESENT)
            else if (device_mode == 1 + active) {
                manage_enter_device();
            }
#endif
            else if (S_ISDIR(current_file_stat.st_mode) || S_ISLNK(current_file_stat.st_mode)) {
                change_dir(ps[active].nl[ps[active].curr_pos]);
            } else {
                manage_file(ps[active].nl[ps[active].curr_pos]);
            }
            break;
        case 't': // t to open second tab
            if (cont < MAX_TABS) {
                cont++;
                restrict_first_tab();
                new_tab(cont - 1);
                change_tab();
            }
            break;
        case 9: // tab to change tab
            if (cont == MAX_TABS) {
                change_tab();
            }
            break;
        case 'w': // w to close second tab
            if ((active) && (sv.searching != 3 + active)) {
                cont--;
                active = 0;
                delete_tab(cont);
                memset(ps[cont].my_cwd, 0, sizeof(ps[cont].my_cwd));
                enlarge_first_tab();
                change_tab();
            }
            break;
        case 32: // space to select files
            if ((sv.searching != 3 + active) && (strcmp(strrchr(ps[active].nl[ps[active].curr_pos], '/') + 1, "..") != 0)) {
                manage_space_press(ps[active].nl[ps[active].curr_pos]);
            }
            break;
        case 'l':  // show helper mess
            help = !help;
            trigger_show_helper_message(help);
            break;
        case 's': // show stat about files (size and perms)
            if (sv.searching != 3 + active) {
                trigger_stats();
            }
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
            if ((sv.searching != 3 + active) && (S_ISREG(current_file_stat.st_mode)) && (!get_mimetype(ps[active].nl[ps[active].curr_pos], "x-executable"))) {
                print_support(ps[active].nl[ps[active].curr_pos]);
            }
            break;
#endif
#if defined(LIBUDEV_PRESENT) && (SYSTEMD_PRESENT)
        case 'm': // m to mount/unmount fs
            if ((sv.searching != 3 + active) && (!device_mode)) {
                devices_tab();
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
        case 'q': /* q to exit/leave search mode/leave device_mode */
            if (sv.searching == 3 + active) {
                leave_search_mode(ps[active].my_cwd);
            }
#if defined(LIBUDEV_PRESENT) && (SYSTEMD_PRESENT)
            else if (device_mode == 1 + active) {
                leave_device_mode();
            }
#endif
            else {
                quit = 1;
            }
            break;
        case KEY_RESIZE:
            resize_win(help);
            break;
        case 'n': case 'd': case 'o':   // fast operations do not require another thread.
            if ((sv.searching != 3 + active) && (check_access())) {
                ptr = strchr(short_table, c);
                index = SHORT_FILE_OPERATIONS - strlen(ptr);
                fast_file_operations(index);
            }
            break;
        default:
            if (sv.searching != 3 + active) {
                ptr = strchr(long_table, c);
                if (ptr) {
                    index = LONG_FILE_OPERATIONS - 1 - strlen(ptr);
                    if (check_init(index)) {
                        init_thread(index, long_func[index]);
                    }
                }
            }
            break;
        }
    }
}

static int check_init(int index)
{
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

static int check_access(void)
{
    if (access(ps[active].my_cwd, W_OK) != 0) {
        print_info(no_w_perm, ERR_LINE);
        return 0;
    }
    return 1;
}
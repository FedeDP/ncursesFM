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
static void main_loop(void);

int main(int argc, const char *argv[])
{
    helper_function(argc, argv);
    #ifdef LIBCONFIG_PRESENT
    read_config_file();
    #endif
    if ((config.starting_dir) && (access(config.starting_dir, F_OK) == -1)) {
        free(config.starting_dir);
        config.starting_dir = NULL;
    }
    screen_init();
    main_loop();
    free_everything();
    screen_end();
    printf("\033c"); // to clear terminal/vt after leaving program
    return 0;
}

static void helper_function(int argc, const char *argv[])
{
    if (argc != 1) {
        if (strcmp(argv[1], "-h") == 0) {
            printf("\tNcursesFM Copyright (C) 2015  Federico Di Pierro (https://github.com/FedeDP):\n");
            printf("\tThis program comes with ABSOLUTELY NO WARRANTY;\n");
            printf("\tThis is free software, and you are welcome to redistribute it under certain conditions;\n");
            printf("\tIt is GPL licensed. Have a look at COPYING file.\n");
            printf("\t\t* -h to view this helper message.\n");
            printf("\t\t* --editor=/path/to/editor to set an editor for current session.\n");
            printf("\t\t* --starting-dir=/path/to/dir to set a starting directory for current session.\n");
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
    int i, j = 1;
    const char *cmd_switch[] = {"--editor=", "--starting-dir="};

    while ((argv[j]) && (j < argc)) {
        i = 0;
        while (i < 2) {
            if (strncmp(cmd_switch[i], argv[j], strlen(cmd_switch[i])) == 0) {
                switch (i) {
                case 0:
                    if ((!config.editor) && (config.editor = safe_malloc((strlen(argv[j]) - strlen(cmd_switch[i])) * sizeof(char) + 1, generic_mem_error))) {
                        strcpy(config.editor, argv[j] + strlen(cmd_switch[i]));
                    }
                    break;
                case 1:
                    if ((!config.starting_dir) && (config.starting_dir = safe_malloc((strlen(argv[j]) - strlen(cmd_switch[i])) * sizeof(char) + 1, generic_mem_error))) {
                        strcpy(config.starting_dir, argv[j] + strlen(cmd_switch[i]));
                    }
                    break;
                }
            }
            i++;
        }
        j++;
    }
    if (!config.editor && !config.starting_dir) {
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
        if (!config.editor && config_lookup_string(&cfg, "editor", &str_editor)) {
            if ((config.editor = safe_malloc(strlen(str_editor) * sizeof(char) + 1, generic_mem_error))) {
                strcpy(config.editor, str_editor);
            }
        }
        config_lookup_int(&cfg, "show_hidden", &config.show_hidden);
        if (!config.starting_dir && config_lookup_string(&cfg, "starting_directory", &str_starting_dir)) {
            if ((config.starting_dir = safe_malloc(strlen(str_starting_dir) * sizeof(char) + 1, generic_mem_error))) {
                strcpy(config.starting_dir, str_starting_dir);
            }
        }
        config_lookup_int(&cfg, "use_default_starting_dir_second_tab", &config.second_tab_starting_dir);
    } else {
        printf("%s", config_file_missing);
        sleep(1);
    }
    config_destroy(&cfg);
}
#endif

static void main_loop(void)
{
    int c;
    char x;
    struct stat current_file_stat;

    while (!quit) {
        c = win_refresh_and_getch();
        stat(ps[active].nl[ps[active].curr_pos], &current_file_stat);
        c = tolower(c);
        switch (c) {
        case KEY_UP:
            scroll_up(ps[active].nl);
            break;
        case KEY_DOWN:
            scroll_down(ps[active].nl);
            break;
        case 'h': // h to show hidden files
            switch_hidden();
            break;
        case 10: // enter to change dir or open a file.
            if (S_ISDIR(current_file_stat.st_mode) || S_ISLNK(current_file_stat.st_mode)) {
                change_dir(ps[active].nl[ps[active].curr_pos]);
            } else {
                manage_file(ps[active].nl[ps[active].curr_pos]);
            }
            break;
        case 't': // t to open second tab
            new_tab();
            break;
        case 9: // tab to change tab
            if (cont == MAX_TABS) {
                change_tab();
            }
            break;
        case 'w': // w to close second tab
            if (active) {
                change_tab();
                delete_tab();
                enlarge_first_tab();
            }
            break;
        case 'n': // new file
            init_thread(NEW_FILE_TH, new_file, ps[active].my_cwd);
            break;
        case 'r': //remove file
            if (strcmp(strrchr(ps[active].nl[ps[active].curr_pos], '/') + 1, "..") != 0) { // check this is not ".." dir
                ask_user(sure, &x, 1, 'n');
                if (x == 'y') {
                    init_thread(RM_TH, remove_file, ps[active].nl[ps[active].curr_pos]);
                }
            }
            break;
        case 'c': case 'x': // copy/cut file
            if (strcmp(strrchr(ps[active].nl[ps[active].curr_pos], '/') + 1, "..") != 0) {
                if (c == 'c') {
                    manage_c_press(0, ps[active].nl[ps[active].curr_pos]);
                } else {
                    manage_c_press(1, ps[active].nl[ps[active].curr_pos]);
                }
            }
            break;
        case 'v': // paste file
            if (selected) {
                init_thread(PASTE_TH, paste_file, ps[active].my_cwd);
            }
            break;
        case 'l':  // show helper mess
            trigger_show_helper_message();
            break;
        case 's': // show stat about files (size and perms)
            show_stats();
            break;
        case 'o': // o to rename
            init_thread(RENAME_TH, rename_file_folders, ps[active].nl[ps[active].curr_pos]);
            break;
        case 'd': // d to create folder
            init_thread(CREATE_DIR_TH, new_file, ps[active].my_cwd);
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
            if (S_ISREG(current_file_stat.st_mode) && !get_mimetype(ps[active].nl[ps[active].curr_pos], "x-executable")) {
                print_support(ps[active].nl[ps[active].curr_pos]);
            }
            break;
        #endif
        case 'b': //b to compress
            if (selected) {
                init_thread(ARCHIVER_TH, create_archive, ps[active].my_cwd);
            }
            break;
        case 'q': /* q to exit */
            quit = 1;
            break;
        }
    }
}
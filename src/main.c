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
#include <libconfig.h>

static void helper_function(int argc, const char *argv[]);
static void init_func(void);
static void main_loop(void);

static const char *config_file_name = "/etc/default/ncursesFM.conf";
// static const char *config_file_name = "/home/federico/ncursesFM/ncursesFM.conf";  // local test entry

int main(int argc, const char *argv[])
{
    helper_function(argc, argv);
    pthread_mutex_init(&lock, NULL);
    init_func();
    screen_init();
    main_loop();
    free_everything();
    screen_end();
    pthread_mutex_destroy(&lock);
    printf("\033c"); // to clear terminal/vt after leaving program
    return 0;
}

static void helper_function(int argc, const char *argv[])
{
    if (argc != 1) {
        if (strcmp(argv[1], "-h") != 0) {
            printf("Use '-h' to view helper message\n");
        } else {
            printf("\tNcursesFM Copyright (C) 2015  Federico Di Pierro (https://github.com/FedeDP):\n");
            printf("\tThis program comes with ABSOLUTELY NO WARRANTY;\n");
            printf("\tThis is free software, and you are welcome to redistribute it under certain conditions;\n");
            printf("\tIt is GPL licensed. Have a look at COPYING file.\n");
            printf("\t\t* Have a look at the config file /etc/default/ncursesFM.conf.\n");
            printf("\t\t* Just use arrow keys to move up and down, and enter to change directory or open a file.\n");
            printf("\t\t* Press 'l' while in program to view a more detailed helper message.\n");
        }
        exit(0);
    }
}

static void init_func(void)
{
    const char *str_editor, *str_starting_dir;
    config_t cfg;

    cont = 0;
    sv.searching = 0;
    current_th = NULL;
    running_h = NULL;
    config.editor = NULL;
    config.starting_dir = NULL;
    config.second_tab_starting_dir = 0;
    config.show_hidden = 0;
    config_init(&cfg);
    if (config_read_file(&cfg, config_file_name)) {
        if (config_lookup_string(&cfg, "editor", &str_editor)) {
            if ((config.editor = safe_malloc(strlen(str_editor) * sizeof(char) + 1, generic_mem_error))) {
                strcpy(config.editor, str_editor);
            }
        }
        config_lookup_int(&cfg, "show_hidden", &config.show_hidden);
        if (config_lookup_string(&cfg, "starting_directory", &str_starting_dir) && access(str_starting_dir, F_OK) != -1) {
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

static void main_loop(void)
{
    int c;
    char x;
    struct stat current_file_stat;

    while (!quit) {
        c = wgetch(ps[active].fm);
        if ((c >= 'A') && (c <= 'Z')) {
            c = tolower(c);
        }
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
            pthread_mutex_lock(&lock);
            stat(ps[active].nl[ps[active].curr_pos], &current_file_stat);
            pthread_mutex_unlock(&lock);
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
            change_tab();
            break;
        case 'w': // w to close second tab
            if (active) {
                change_tab();
                delete_tab();
            }
            break;
        case 'n': // new file
            init_thread(NEW_FILE_TH, new_file, ps[active].my_cwd);
            break;
        case 'r': //remove file
            if (strcmp(strrchr(ps[active].nl[ps[active].curr_pos], '/') + 1, "..") != 0) {
                ask_user(sure, &x, 1, 'n');
                if (x == 'y') {
                    init_thread(RM_TH, remove_file, ps[active].nl[ps[active].curr_pos]);
                }
            }
            break;
        case 'c': case 'x': // copy/cut file
            if (strcmp(strrchr(ps[active].nl[ps[active].curr_pos], '/') + 1, "..") != 0) {
                manage_c_press(c);
            }
            break;
        case 'v': // paste file
            if (current_th && current_th->selected_files) {
                init_thread(PASTE_TH, paste_file, ps[active].my_cwd);
            }
            break;
        case 'l':
            trigger_show_helper_message();
            break;
        case 's': // show stat about files (size and perms)
            ps[active].stat_active = !ps[active].stat_active;
            if (ps[active].stat_active) {
                list_everything(active, ps[active].delta, 0, ps[active].nl);
            } else {
                erase_stat();
            }
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
            pthread_mutex_lock(&lock);
            stat(ps[active].nl[ps[active].curr_pos], &current_file_stat);
            pthread_mutex_unlock(&lock);
            if (S_ISREG(current_file_stat.st_mode) && !get_mimetype(ps[active].nl[ps[active].curr_pos], "x-executable")) {
                print_support(ps[active].nl[ps[active].curr_pos]);
            }
            break;
        #endif
        case 'b': //b to compress
            if (current_th && current_th->selected_files) {
                init_thread(ARCHIVER_TH, create_archive, ps[active].my_cwd);
            }
            break;
        case 'u': // u to view mimetype
            get_mimetype(ps[active].nl[ps[active].curr_pos], NULL);
            break;
        case 'q': /* q to exit */
            quit = 1;
            break;
        }
    }
}
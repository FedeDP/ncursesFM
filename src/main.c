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
#include <unistd.h>

static void helper_function(int argc, char *argv[]);
static void init_func(void);
static void main_loop(int *quit, int *old_number_files);

static const char *config_file_name = "/etc/default/ncursesFM.conf";
//static const char *config_file_name = "/home/federico/ncursesFM/ncursesFM.conf";  // local test entry

int main(int argc, char *argv[])
{
    int quit = 0, old_number_files;
    helper_function(argc, argv);
    init_func();
    screen_init();
    while (!quit)
        main_loop(&quit, &old_number_files);
    free_everything();
    screen_end();
    return 0;
}

static void helper_function(int argc, char *argv[])
{
    if (argc != 1) {
        if (strcmp(argv[1], "-h") != 0)
            printf("Use '-h' to view helper message\n");
        else {
            printf("\tNcursesFM Copyright (C) 2015  Federico Di Pierro (https://github.com/FedeDP):\n");
            printf("\tThis program comes with ABSOLUTELY NO WARRANTY;\n");
            printf("\tThis is free software, and you are welcome to redistribute it under certain conditions;\n");
            printf("\tIt is GPL licensed. Have a look at COPYING file.\n");
            printf("\t\t* Just use arrow keys to move up and down, and enter to change directory or open a file.\n");
            printf("\t\t* Press 'l' while in program to view a more detailed helper message.\n");
        }
        exit(0);
    }
}

static void init_func(void)
{
    const char *str_editor, *str_hidden, *str_starting_dir;
    config_t cfg;
    cont = 0;
    search_mode = 0;
    searching = 0;
    selected_files = NULL;
    config.editor = NULL;
    config.starting_dir = NULL;
    config.second_tab_starting_dir = 0;
    config_init(&cfg);
    if (config_read_file(&cfg, config_file_name)) {
        if (config_lookup_string(&cfg, "editor", &str_editor)) {
            config.editor = malloc(strlen(str_editor) * sizeof(char) + 1);
            strcpy(config.editor, str_editor);
        }
        if (!(config_lookup_int(&cfg, "show_hidden", &config.show_hidden)))
            config.show_hidden = 0;
        if ((config_lookup_string(&cfg, "starting_directory", &str_starting_dir)) && (access(str_starting_dir, F_OK) != -1)) {
            config.starting_dir = malloc(strlen(str_starting_dir) * sizeof(char) + 1);
            strcpy(config.starting_dir, str_starting_dir);
        }
        config_lookup_int(&cfg, "use_default_starting_dir_second_tab", &config.second_tab_starting_dir);
    } else {
        printf("Config file not found. Check /etc/default/ncursesFM.conf. Using default values.\n");
        sleep(1);
    }
    config_destroy(&cfg);
}

static void main_loop(int *quit, int *old_number_files)
{
    int c;
    struct stat file_stat;
    stat(ps[active].nl[ps[active].curr_pos], &file_stat);
    c = wgetch(ps[active].fm);
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
            if ((S_ISDIR(file_stat.st_mode)) || (S_ISLNK(file_stat.st_mode)))
                change_dir(ps[active].nl[ps[active].curr_pos]);
            else
                manage_file(ps[active].nl[ps[active].curr_pos]);
            break;
        case 't': // t to open second tab
            if (cont < MAX_TABS)
                new_tab();
            break;
        case 9: // tab to change tab
            if (cont == MAX_TABS) {
                active = 1 - active;
                chdir(ps[active].my_cwd);
            }
            break;
        case 'w': //close ps.active new_tab
            if (active != 0)
                delete_tab();
            break;
        case 'n': // new file
            new_file();
            break;
        case 'r': //remove file
            if (strcmp(ps[active].nl[ps[active].curr_pos], "..") != 0)
                remove_file();
            break;
        case 'c': case 'x': // copy/cut file
            if (strcmp(ps[active].nl[ps[active].curr_pos], "..") != 0)
                manage_c_press(c);
            break;
        case 'v': // paste file
            if (selected_files)
                paste_file();
            break;
        case 'l':
            trigger_show_helper_message();
            break;
        case 's': // show stat about files (size and perms)
            ps[active].stat_active = 1 - ps[active].stat_active;
            if (ps[active].stat_active == 1)
                list_everything(active, ps[active].delta, dim - 2, ps[active].nl);
            else
                erase_stat();
            break;
        case 'o': // o to rename
            rename_file_folders();
            break;
        case 'd': // d to create folder
            create_dir();
            break;
        case 'f': // f to search
            if (searching == 0)
                search();
            else {
                if (searching == 2)
                    list_found();
            }
            break;
        case 'p': // p to print
            if (S_ISREG(file_stat.st_mode))
                print_support(ps[active].nl[ps[active].curr_pos]);
            break;
        case 'b': //b to compress
            if (selected_files)
                create_archive();
            break;
        case 'q': /* q to exit */
            quit_thread_func();
            *quit = 1;
            break;
    }
}
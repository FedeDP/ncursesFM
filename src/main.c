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

void helper_function(int argc, char *argv[]);
void init_func(void);
void main_loop(int *quit, int *old_number_files);

// static const char *config_file_name = "/etc/default/ncursesFM.conf";
static const char *config_file_name = "ncursesFM.conf";

int main(int argc, char *argv[])
{
    int quit = 0, old_number_files;
    config.editor = NULL;
    config.show_hidden = 0;
    config.iso_mount_point = NULL;
    config.starting_dir = NULL;
    ps.active = 0;
    ps.cont = 1;
    ps.copied_files = NULL;
    ps.pasted = 0;
    helper_function(argc, argv);
    init_func();
    screen_init();
    if ((config.starting_dir) && (access(config.starting_dir, F_OK) != -1)) {
        strcpy(ps.my_cwd[ps.active], config.starting_dir);
    } else {
        if (access(config.starting_dir, F_OK) == -1)
            print_info("Check starting_directory entry in config file. The directory currently specified thas't exist.", INFO_LINE);
        getcwd(ps.my_cwd[ps.active], PATH_MAX);
    }
    list_everything(ps.active, 0, dim - 2, 1, 1);
    while (!quit)
        main_loop(&quit, &old_number_files);
    free_everything();
    screen_end();
    return 0;
}

/* UI functions */
void helper_function(int argc, char *argv[])
{
    if (argc == 1)
        return;
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

void init_func(void)
{
    config_t cfg;
    const char *str_editor, *str_hidden, *str_starting_dir;
    config_init(&cfg);
    if (!config_read_file(&cfg, config_file_name)) {
        printf("\n%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        exit(1);
    }
    if (config_lookup_string(&cfg, "editor", &str_editor)) {
        config.editor = malloc(strlen(str_editor) * sizeof(char) + 1);
        strcpy(config.editor, str_editor);
    }
    config_lookup_int(&cfg, "show_hidden", &config.show_hidden);
    if (config.show_hidden != 0) {
        if (config.show_hidden != 1)
            config.show_hidden = 1;
    }
    if (config_lookup_string(&cfg, "iso_mount_point", &str_hidden)) {
        config.iso_mount_point = malloc(strlen(str_hidden) * sizeof(char) + 1);
        strcpy(config.iso_mount_point, str_hidden);
    }
    if (config_lookup_string(&cfg, "starting_directory", &str_starting_dir)) {
        config.starting_dir = malloc(strlen(str_starting_dir) * sizeof(char) + 1);
        strcpy(config.starting_dir, str_starting_dir);
    }
    config_destroy(&cfg);
}

/* FM functions */
void main_loop(int *quit, int *old_number_files)
{
    int c;
    if (ps.pasted == 1)
        check_pasted();
    c = wgetch(file_manager[ps.active]);
    switch (c) {
        case KEY_UP:
            scroll_up(NULL);
            break;
        case KEY_DOWN:
            scroll_down(NULL);
            break;
        case 'h': // h to show hidden files
            switch_hidden();
            break;
        case 10: // enter to change dir or open a file.
            if ((namelist[ps.active][ps.current_position[ps.active]]->d_type ==  DT_DIR) || (namelist[ps.active][ps.current_position[ps.active]]->d_type ==  DT_LNK))
                change_dir(namelist[ps.active][ps.current_position[ps.active]]->d_name);
            else
                manage_file(namelist[ps.active][ps.current_position[ps.active]]->d_name);
            break;
        case 't': // t to open second tab
            if (ps.cont < MAX_TABS)
                new_tab();
            break;
        case 9: // tab to change tab
            if (ps.cont == MAX_TABS) {
                ps.active = 1 - ps.active;
                chdir(ps.my_cwd[ps.active]);
            }
            break;
        case 'w': //close ps.active new_tab
            if (ps.active != 0)
                delete_tab();
            break;
        case 'n': // new file
            new_file();
            break;
        case 'r': //remove file
            if (strcmp(namelist[ps.active][ps.current_position[ps.active]]->d_name, "..") != 0)
                remove_file();
            break;
        case 'c': case 'x': // copy file
            if (strcmp(namelist[ps.active][ps.current_position[ps.active]]->d_name, "..") != 0)
                manage_c_press(c);
            break;
        case 'v': // paste file
            if (ps.copied_files) {
                set_nodelay(TRUE);
                paste_file();
            }
            break;
        case 'l':
            trigger_show_helper_message();
            break;
        case 's': // show stat about files (size and perms)
            ps.stat_active[ps.active] = 1 - ps.stat_active[ps.active];
            list_everything(ps.active, ps.delta[ps.active], dim - 2, 1, 0);
            break;
        case 'o': // o to rename
            rename_file_folders();
            break;
        case 'd': // d to create folder
            create_dir();
            break;
        case 'f': // b to search
            search();
            break;
        case 'p': // p to print
            if (namelist[ps.active][ps.current_position[ps.active]]->d_type == DT_REG)
                print_support(namelist[ps.active][ps.current_position[ps.active]]->d_name);
            break;
        case 'q': /* q to exit */
            quit_func();
            *quit = 1;
            break;
    }
}
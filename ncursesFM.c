/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * NcursesFM: file manager in C with ncurses UI for linux.
 * https://github.com/FedeDP/ncursesFM
 *
 * Copyright (C) 2015  Federico Di Pierro <nierro92@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
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


#define _GNU_SOURCE
#include <ftw.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <libconfig.h>
#include <errno.h>
#include <pthread.h>

#define INITIAL_POSITION 1
#define MAX_TABS 2
#define INFO_LINE 0
#define ERR_LINE 1
#define INFO_HEIGHT 2
#define HELPER_HEIGHT 13
#define STAT_COL 30
#define MAX_FILENAME_LENGTH 25

struct conf {
    char *editor;
    int show_hidden;
    char *iso_mount_point;
    char *starting_dir;
};

typedef struct list {
    char copied_file[PATH_MAX];
    char copied_dir[PATH_MAX];
    int cut;
    struct list *next;
} copied_file_list;

struct vars {
    int current_position[MAX_TABS];
    int number_of_files[MAX_TABS];
    int active;
    int cont;
    int delta[MAX_TABS];
    int stat_active[MAX_TABS];
    copied_file_list *copied_files;
    char pasted_dir[PATH_MAX];
    char my_cwd[MAX_TABS][PATH_MAX];
    int pasted;
};
/* UI functions */
static void helper_function(int argc, char *argv[]);
static void init_func(void);
static void screen_init(void);
static void screen_end(void);
static void list_everything(int win, int old_dim, int end, int erase, int reset);
static int is_hidden(const struct dirent *current_file);
static void my_sort(int win);
static void new_tab(void);
static void delete_tab(void);
static void scroll_down(char *str);
static void scroll_up(char *str);
static void scroll_helper_func(int x, int direction);
static void sync_changes(void);
static void colored_folders(int i, int win);
static void print_info(char *str, int i);
static void clear_info(int i);
static void trigger_show_helper_message(void);
static void helper_print(void);
static void show_stat(int init, int end, int win);
static void set_nodelay(bool x);
/* FM functions */
static void main_loop(int *quit, int *old_number_files);
static void change_dir(char *str);
static void switch_hidden(void);
static void manage_file(char *str);
static void open_file(char *str);
static void iso_mount_service(char *str);
static void new_file(void);
static void remove_file(void);
static void manage_c_press(char c);
static int remove_from_list(char *name);
static void copy_file(char c);
static void paste_file(void);
static void *cpr(void *x);
static void check_pasted(void);
static void rename_file_folders(void);
static void create_dir(void);
static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int rmrf(char *path);
static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int search_file(char *path);
static void search(void);
static int search_loop(int size);
static void print_support(char *str);
/* Helper functions */
static int isIso(char *ext);
static int file_isCopied(void);
static void get_full_path(char *full_path_current_position);
static int ask_user(char *str);
/* Quit functions */
static void free_copied_list(copied_file_list *h);
static void free_everything(void);
static void quit_func(void);

// static const char *config_file_name = "/etc/default/ncursesFM.conf";
static const char *config_file_name = "ncursesFM.conf";
static const char *iso_extensions[] = {".iso", ".bin", ".nrg", ".img", ".mdf"};
static WINDOW *file_manager[MAX_TABS], *info_win, *helper_win = NULL;
static int dim;
static struct dirent **namelist[MAX_TABS] = {NULL, NULL};
static char *found_searched[PATH_MAX];
static char searched_string[PATH_MAX];
static pthread_t th;
static struct conf config = {
    .editor = NULL,
    .show_hidden = 0,
    .iso_mount_point = NULL,
    .starting_dir = NULL
};
static struct vars ps = {
    .active = 0,
    .cont = 1,
    .delta = {0, 0},
    .stat_active = {0, 0},
    .copied_files = NULL,
    .pasted = 0
};

int main(int argc, char *argv[])
{
    int quit = 0, old_number_files;
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
static void helper_function(int argc, char *argv[])
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

static void init_func(void)
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

static void screen_init(void)
{
    initscr();
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    raw();
    noecho();
    curs_set(0);
    dim = LINES - INFO_HEIGHT;
    file_manager[ps.active] = subwin(stdscr, dim, COLS, 0, 0);
    scrollok(file_manager[ps.active], TRUE);
    idlok(file_manager[ps.active], TRUE);
    info_win = subwin(stdscr, INFO_HEIGHT, COLS, dim, 0);
    keypad(file_manager[ps.active], TRUE);
}

static void screen_end(void)
{
    int i;
    for (i = 0; i < ps.cont; i++) {
        wclear(file_manager[i]);
        delwin(file_manager[i]);
    }
    if (helper_win) {
        wclear(helper_win);
        delwin(helper_win);
    }
    wclear(info_win);
    delwin(info_win);
    endwin();
    delwin(stdscr);
}

static void list_everything(int win, int old_dim, int end, int erase, int reset)
{
    int i;
    chdir(ps.my_cwd[win]);
    if (erase == 1) {
        for (i = 0; i < ps.number_of_files[win]; i++)
            free(namelist[win][i]);
        free(namelist[win]);
        namelist[win] = NULL;
        wclear(file_manager[win]);
        if (reset == 1) {
            ps.delta[win] = 0;
            ps.current_position[win] = 0;
        }
        ps.number_of_files[win] = scandir(ps.my_cwd[win], &namelist[win], is_hidden, alphasort);
        my_sort(win);
    }

    wborder(file_manager[win], '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(file_manager[win], 0, 0, "Current:%.*s", COLS / ps.cont - 1 - strlen("Current:"), ps.my_cwd[win]);
    for (i = old_dim; (i < ps.number_of_files[win]) && (i  < old_dim + end); i++) {
        colored_folders(i, win);
        mvwprintw(file_manager[win], INITIAL_POSITION + i - ps.delta[win], 4, "%.*s", MAX_FILENAME_LENGTH, namelist[win][i]->d_name);
        wattroff(file_manager[win], COLOR_PAIR(1));
        wattroff(file_manager[win], A_BOLD);
    }
    mvwprintw(file_manager[win], INITIAL_POSITION + ps.current_position[win] - ps.delta[win], 1, "->");
    if (ps.stat_active[win] == 1)
        show_stat(old_dim, end, win);
    wrefresh(file_manager[win]);
}

int is_hidden(const struct dirent *current_file)
{
    if ((strlen(current_file->d_name) == 1) && (current_file->d_name[0] == '.'))
        return (FALSE);
    if (config.show_hidden == 0) {
        if ((strlen(current_file->d_name) > 1) && (current_file->d_name[0] == '.') && (current_file->d_name[1] != '.'))
            return (FALSE);
        return (TRUE);
    }
    return (TRUE);
}

static void my_sort(int win)
{
    struct dirent *temp;
    int i, j;
    for (i = 0; i < ps.number_of_files[win] - 1; i++) {
        if (namelist[win][i]->d_type !=  DT_DIR) {
            for (j = i + 1; j < ps.number_of_files[win]; j++) {
                if (namelist[win][j]->d_type == DT_DIR) {
                    temp = namelist[win][j];
                    namelist[win][j] = namelist[win][i];
                    namelist[win][i] = temp;
                    break;
                }
            }
        }
    }
}

static void new_tab(void)
{
    ps.cont++;
    wresize(file_manager[ps.active], dim, COLS / ps.cont);
    wborder(file_manager[ps.active], '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(file_manager[ps.active], 0, 0, "Current:%.*s", COLS / ps.cont - 1 - strlen("Current:"), ps.my_cwd[ps.active]);
    wrefresh(file_manager[ps.active]);
    ps.active = 1;
    file_manager[ps.active] = subwin(stdscr, dim, COLS / ps.cont, 0, COLS / ps.cont);
    keypad(file_manager[ps.active], TRUE);
    scrollok(file_manager[ps.active], TRUE);
    idlok(file_manager[ps.active], TRUE);
    getcwd(ps.my_cwd[ps.active], PATH_MAX);
    list_everything(ps.active, 0, dim - 2, 1, 1);
}

static void delete_tab(void)
{
    int i;
    ps.cont--;
    wclear(file_manager[ps.active]);
    delwin(file_manager[ps.active]);
    file_manager[ps.active] = NULL;
    for (i = ps.number_of_files[ps.active] - 1; i >= 0; i--)
        free(namelist[ps.active][i]);
    free(namelist[ps.active]);
    namelist[ps.active] = NULL;
    ps.number_of_files[ps.active] = 0;
    ps.stat_active[ps.active] = 0;
    memset(ps.my_cwd[ps.active], 0, sizeof(ps.my_cwd[ps.active]));
    ps.active = 0;
    wborder(file_manager[ps.active], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wresize(file_manager[ps.active], dim, COLS);
    wborder(file_manager[ps.active], '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(file_manager[ps.active], 0, 0, "Current:%.*s", COLS / ps.cont - 1 - strlen("Current:"), ps.my_cwd[ps.active]);
}

static void scroll_down(char *str)
{
    int real_height = dim - 2;
    ps.current_position[ps.active]++;
    if (ps.current_position[ps.active] == ps.number_of_files[ps.active]) {
        ps.current_position[ps.active]--;
        return;
    }
    if (ps.current_position[ps.active] - real_height == ps.delta[ps.active]) {
        scroll_helper_func(real_height, 1);
        ps.delta[ps.active]++;
        if (!str)
            list_everything(ps.active, ps.current_position[ps.active], 1, 0, 0);
        else {
            mvwprintw(file_manager[ps.active], real_height, 4, str);
            mvwprintw(file_manager[ps.active], INITIAL_POSITION + ps.current_position[ps.active] - ps.delta[ps.active], 1, "->");
        }
    } else {
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active], 1, "  ");
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active] + INITIAL_POSITION, 1, "->");
    }
}

static void scroll_up(char *str)
{
    ps.current_position[ps.active]--;
    if (ps.current_position[ps.active] < 0) {
        ps.current_position[ps.active]++;
        return;
    }
    if (ps.current_position[ps.active] < ps.delta[ps.active]) {
        scroll_helper_func(INITIAL_POSITION, - 1);
        ps.delta[ps.active]--;
        if (!str)
            list_everything(ps.active, ps.delta[ps.active], 1, 0, 0);
        else {
            mvwprintw(file_manager[ps.active], INITIAL_POSITION, 4, str);
            mvwprintw(file_manager[ps.active], INITIAL_POSITION + ps.current_position[ps.active] - ps.delta[ps.active], 1, "->");
        }
    } else {
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active] + 2, 1, "  ");
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active] + 1, 1, "->");
    }
}

static void scroll_helper_func(int x, int direction)
{
    mvwprintw(file_manager[ps.active], x, 1, "  ");
    wborder(file_manager[ps.active], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wscrl(file_manager[ps.active], direction);
}

static void sync_changes(void)
{
    if (ps.cont == 2) {
        if (strcmp(ps.my_cwd[ps.active], ps.my_cwd[1 - ps.active]) == 0)
            list_everything(1 - ps.active, 0, dim - 2, 1, 1);
    }
}

static void colored_folders(int i, int win)
{
    struct stat file_stat;
    wattron(file_manager[win], A_BOLD);
    if (namelist[win][i]->d_type == DT_DIR)
        wattron(file_manager[win], COLOR_PAIR(1));
    else if (namelist[win][i]->d_type == DT_LNK)
        wattron(file_manager[win], COLOR_PAIR(3));
    else if (namelist[win][i]->d_type == DT_REG) {
        stat(namelist[win][i]->d_name, &file_stat);
        if (file_stat.st_mode & S_IXUSR)
            wattron(file_manager[win], COLOR_PAIR(2));
    }
}

static void print_info(char *str, int i)
{
    clear_info(i);
    mvwprintw(info_win, i, 1, str);
    wrefresh(info_win);
}

static void clear_info(int i)
{
    wmove(info_win, i, 1);
    wclrtoeol(info_win);
    if ((ps.copied_files) && (ps.pasted == 0))
        mvwprintw(info_win, INFO_LINE, COLS - strlen("File added to copy list."), "File added to copy list.");
    else if (ps.pasted == -1)
        mvwprintw(info_win, INFO_LINE, COLS - strlen("Pasting files..."), "Pasting_files...");
    wrefresh(info_win);
}

static void trigger_show_helper_message(void)
{
    int i;
    if (helper_win == NULL) {
        dim = LINES - INFO_HEIGHT - HELPER_HEIGHT;
        for (i = 0; i < ps.cont; i++) {
            wresize(file_manager[i], dim, COLS / ps.cont);
            wborder(file_manager[i], '|', '|', '-', '-', '+', '+', '+', '+');
            mvwprintw(file_manager[i], 0, 0, "Current:%.*s", COLS / ps.cont - 1 - strlen("Current:"), ps.my_cwd[i]);
            if (ps.current_position[i] > dim - 3) {
                ps.current_position[i] = dim - 3 + ps.delta[i];
                mvwprintw(file_manager[i], ps.current_position[i] - ps.delta[i] + INITIAL_POSITION, 1, "->");
            }
            wrefresh(file_manager[i]);
        }
        helper_win = subwin(stdscr, HELPER_HEIGHT, COLS, LINES - INFO_HEIGHT - HELPER_HEIGHT, 0);
        wclear(helper_win);
        helper_print();
        wrefresh(helper_win);
    } else {
        wclear(helper_win);
        delwin(helper_win);
        helper_win = NULL;
        dim = LINES - INFO_HEIGHT;
        for (i = 0; i < ps.cont; i++) {
            wborder(file_manager[i], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
            wresize(file_manager[i], dim, COLS / ps.cont);
            list_everything(i, dim - 2 - HELPER_HEIGHT + ps.delta[i], HELPER_HEIGHT, 0, 0);
        }
    }
}

static void helper_print(void)
{
    wprintw(helper_win, "\n HELPER MESSAGE:\n * n and r to create/remove a file.\n");
    wprintw(helper_win, " * Enter to surf between folders or to open files with $editor var.\n");
    wprintw(helper_win, " * Enter will eventually mount your ISO files in $path.\n");
    wprintw(helper_win, " * You must have fuseiso installed. To unmount, simply press again enter on the same iso file.\n");
    wprintw(helper_win, " * Press h to trigger the showing of hide files. s to see stat about files in current folder.\n");
    wprintw(helper_win, " * c to copy, p to paste, and x to cut a file/dir. p to print a file.\n");
    wprintw(helper_win, " * You can copy as many files/dirs as you want. c again on a file/dir to remove it from copy list.\n");
    wprintw(helper_win, " * o to rename current file/dir; d to create new dir. f to search (case sensitive) for a file.\n");
    wprintw(helper_win, " * t to create new tab (at most one more). w to close tab.\n");
    wprintw(helper_win, " * You can't close first tab. Use q to quit.");
    wborder(helper_win, '|', '|', '-', '-', '+', '+', '+', '+');
}

static void show_stat(int init, int end, int win)
{
    int i = init;
    unsigned long total_size = 0;
    struct stat file_stat;
    if (init == 0) {
        for (i = 1; i < ps.number_of_files[win]; i++) {
            stat(namelist[win][i]->d_name, &file_stat);
            total_size = total_size + file_stat.st_size;
        }
        mvwprintw(file_manager[win], INITIAL_POSITION, STAT_COL, "Total size: %lu KB", total_size / 1024);
        i = 1;
    }
    for (; ((i < init + end) && (i < ps.number_of_files[win])); i++) {
        if (stat(namelist[win][i]->d_name, &file_stat) == - 1) { //debug
            mvwprintw(file_manager[win], i + INITIAL_POSITION - ps.delta[win], STAT_COL, "%s/%s %s", ps.my_cwd[win], namelist[win][i]->d_name, strerror(errno));
            return;
        }
        mvwprintw(file_manager[win], i + INITIAL_POSITION - ps.delta[win], STAT_COL, "Size: %d KB", file_stat.st_size / 1024);
        wprintw(file_manager[win], "\tPerm: ");
        wprintw(file_manager[win], (S_ISDIR(file_stat.st_mode)) ? "d" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IRUSR) ? "r" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IWUSR) ? "w" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IXUSR) ? "x" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IRGRP) ? "r" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IWGRP) ? "w" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IXGRP) ? "x" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IROTH) ? "r" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IWOTH) ? "w" : "-");
        wprintw(file_manager[win], (file_stat.st_mode & S_IXOTH) ? "x" : "-");
    }
}

static void set_nodelay(bool x)
{
    int i;
    for (i = 0; i < ps.cont; i++)
        nodelay(file_manager[i], x);
}

/* FM functions */
static void main_loop(int *quit, int *old_number_files)
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

static void change_dir(char *str)
{
    chdir(str);
    getcwd(ps.my_cwd[ps.active], PATH_MAX);
    list_everything(ps.active, 0, dim - 2, 1, 1);
    clear_info(INFO_LINE);
    clear_info(ERR_LINE);
}

static void switch_hidden(void)
{
    int i;
    config.show_hidden = 1 - config.show_hidden;
    for (i = 0; i < ps.cont; i++)
        list_everything(i, 0, dim - 2, 1, 1);
}

static void manage_file(char *str)
{
    char *ext;
    if (file_isCopied())
        return;
    ext = strrchr(str, '.');
    if ((ext) && (isIso(ext))) {
        if (config.iso_mount_point)
            iso_mount_service(str);
        else
            print_info("You have to specify an iso mount point in config file.", ERR_LINE);
    } else {
        if (config.editor)
            open_file(str);
        else
            print_info("You have to specify an editor in config file.", ERR_LINE);
    }
}

static void open_file(char *str)
{
    pid_t pid;
    endwin();
    pid = vfork();
    if (pid == 0)
        execl(config.editor, config.editor, str, NULL);
    else
        waitpid(pid, NULL, 0);
    refresh();
}

static void iso_mount_service(char *str)
{
    pid_t pid;
    char mount_point[strlen(str) - 4 + strlen(config.iso_mount_point)];
    strcpy(mount_point, config.iso_mount_point);
    strcat(mount_point, "/");
    strncat(mount_point, str, strlen(str) - 4);
    if (access("/usr/bin/fuseiso", F_OK ) != -1) {
        pid = vfork();
        if (pid == 0) {
            if (mkdir(mount_point, ACCESSPERMS) == -1) {
                execl("/usr/bin/fusermount", "/usr/bin/fusermount", "-u", mount_point, NULL);
            } else
                execl("/usr/bin/fuseiso", "/usr/bin/fuseiso", str, mount_point, NULL);
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        print_info("You need fuseiso for iso mounting.", ERR_LINE);
    }
    rmdir(mount_point);
    if (strcmp(config.iso_mount_point, ps.my_cwd[ps.active]) == 0)
        list_everything(ps.active, 0, dim - 2, 1, 1);
    if (strcmp(config.iso_mount_point, ps.my_cwd[1 - ps.active]) == 0)
        list_everything(1 - ps.active, 0, dim - 2, 1, 1);
    chdir(ps.my_cwd[ps.active]);
}

static void new_file(void)
{
    FILE *f;
    char *mesg = "Insert new file name:> ", str[PATH_MAX];
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, str);
    f = fopen(str, "w");
    if (!f) {
        print_info(strerror(errno), ERR_LINE);
    } else {
        fclose(f);
        list_everything(ps.active, 0, dim - 2, 1, 1);
        sync_changes();
        print_info("File created.", INFO_LINE);
    }
    noecho();
}

static void remove_file(void)
{
    char *mesg = "Are you serious? y/n:> ";
    if (file_isCopied())
        return;
    if (ask_user(mesg) == 1) {
        if (rmrf(namelist[ps.active][ps.current_position[ps.active]]->d_name) == -1)
            print_info("Could not remove. Check user permissions.", ERR_LINE);
        else {
            list_everything(ps.active, 0, dim - 2, 1, 1);
            sync_changes();
            print_info("File/dir removed.", INFO_LINE);
        }
    }
}

static void manage_c_press(char c)
{
    if (remove_from_list(namelist[ps.active][ps.current_position[ps.active]]->d_name) == 0)
        copy_file(c);
    else {
        if (ps.copied_files)
            print_info("File deleted from copy list.", INFO_LINE);
        else
            print_info("File deleted from copy list. Copy list empty.", INFO_LINE);
    }
}

static int remove_from_list(char *name)
{
    copied_file_list *tmp = ps.copied_files, *temp = NULL;
    char str[PATH_MAX];
    if (!ps.copied_files)
        return 0;
    strcpy(str, strrchr(ps.copied_files->copied_file, '/'));
    memmove(str, str + 1, strlen(str));
    if (strcmp(str, name) == 0) {
        ps.copied_files = ps.copied_files->next;
        free(tmp);
        return 1;
    }
    memset(str, 0, strlen(str));
    while(tmp->next) {
        strcpy(str, strrchr(tmp->next->copied_file, '/'));
        memmove(str, str + 1, strlen(str) - 2);
        if (strcmp(str, name) == 0) {
            temp = tmp->next;
            tmp->next = tmp->next->next;
            free(temp);
            return 1;
        }
        tmp = tmp->next;
        memset(str, 0, strlen(str));
    }
    return 0;
}

static void copy_file(char c)
{
    copied_file_list *tmp;
    if (ps.copied_files) {
        tmp = ps.copied_files;
        while(tmp->next)
            tmp = tmp->next;
        tmp->next = malloc(sizeof(struct list));
        get_full_path(tmp->next->copied_file);
        strcpy(tmp->next->copied_dir, ps.my_cwd[ps.active]);
        tmp->next->cut = 0;
        if (c == 'x')
            tmp->next->cut = 1;
        tmp->next->next = NULL;
    } else {
        ps.copied_files = malloc(sizeof(struct list));
        get_full_path(ps.copied_files->copied_file);
        strcpy(ps.copied_files->copied_dir, ps.my_cwd[ps.active]);
        ps.copied_files->cut = 0;
        if (c == 'x')
            ps.copied_files->cut = 1;
        ps.copied_files->next = NULL;
    }
    mvwprintw(info_win, INFO_LINE, COLS - strlen("File added to copy list."), "File added to copy list.");
    wrefresh(info_win);
}

static void paste_file(void)
{
    char pasted_file[PATH_MAX];
    int size = 0;
    struct stat file_stat_copied, file_stat_pasted;
    copied_file_list *tmp = ps.copied_files;
    strcpy(ps.pasted_dir, ps.my_cwd[ps.active]);
    if (access(ps.pasted_dir, W_OK) == 0) {
        stat(ps.pasted_dir, &file_stat_pasted);
        while (tmp) {
            if (strcmp(ps.pasted_dir, tmp->copied_dir) != 0) {
                size++;
                stat(tmp->copied_dir, &file_stat_copied);
                if ((file_stat_copied.st_dev == file_stat_pasted.st_dev) && (tmp->cut == 1)) {
                    strcpy(pasted_file, ps.pasted_dir);
                    strcat(pasted_file, strrchr(tmp->copied_file, '/'));
                    tmp->cut = -1;
                    size--;
                    if (rename(tmp->copied_file, pasted_file) == - 1)
                        print_info(strerror(errno), ERR_LINE);
                }
            } else {
                print_info("Cannot copy a file in the same folder.", ERR_LINE);
                remove_from_list(tmp->copied_file);
            }
            tmp = tmp->next;
        }
        if (size > 0) {
            wmove(info_win, INFO_LINE, 1);
            wclrtoeol(info_win);
            mvwprintw(info_win, INFO_LINE, COLS - strlen("Pasting files..."), "Pasting_files...");
            wrefresh(info_win);
            pthread_create(&th, NULL, cpr, NULL);
            pthread_detach(th);
        } else {
            ps.pasted = 1;
        }
    } else {
        wclear(info_win);
        mvwprintw(info_win, ERR_LINE, 1, "Cannot copy here. Check user permissions.");
        wrefresh(info_win);
        free_copied_list(ps.copied_files);
        ps.copied_files = NULL;
        memset(ps.pasted_dir, 0, strlen(ps.pasted_dir));
        set_nodelay(FALSE);
    }
}


static void *cpr(void *x)
{
    pid_t pid;
    int status;
    copied_file_list *tmp = ps.copied_files;
    ps.pasted = -1;
    while (tmp) {
        if (tmp->cut != -1) {
            pid = vfork();
            if (pid == 0)
                execl("/usr/bin/cp", "/usr/bin/cp", "-r", tmp->copied_file, ps.pasted_dir, NULL);
            else
                waitpid(pid, &status, 0);
        }
        tmp = tmp->next;
    }
    ps.pasted = 1;
    return NULL;
}

static void check_pasted(void)
{
    int i, printed[ps.cont];
    copied_file_list *tmp = ps.copied_files;
    for (i = 0; i < ps.cont; i++) {
        if (strcmp(ps.pasted_dir, ps.my_cwd[i]) == 0) {
            list_everything(i, 0, dim - 2, 1, 1);
            printed[i] = 1;
        } else {
            printed[i] = 0;
        }
    }
    while (tmp) {
        if (tmp->cut == 1)
            if (rmrf(tmp->copied_file) == -1) {
                print_info("Could not cut. Check user permissions.", ERR_LINE);
            }
        if ((tmp->cut == 1) || (tmp->cut == -1)) {
            for (i = 0; i < ps.cont; i++) {
                if ((printed[i] == 0) && (strcmp(tmp->copied_dir, ps.my_cwd[i]) == 0))
                    list_everything(i, 0, dim - 2, 1, 1);
            }
        }
        tmp = tmp->next;
    }
    chdir(ps.my_cwd[ps.active]);
    print_info("Every files has been copied/moved.", INFO_LINE);
    free_copied_list(ps.copied_files);
    ps.copied_files = NULL;
    memset(ps.pasted_dir, 0, strlen(ps.pasted_dir));
    ps.pasted = 0;
    set_nodelay(FALSE);
}


static void rename_file_folders(void)
{
    char *mesg = "Insert new name:> ", str[PATH_MAX];
    if (file_isCopied())
        return;
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, str);
    noecho();
    if (rename(namelist[ps.active][ps.current_position[ps.active]]->d_name, str) == - 1) {
        print_info(strerror(errno), ERR_LINE);
    } else {
        list_everything(ps.active, 0, dim - 2, 1, 1);
        sync_changes();
        print_info("File renamed.", INFO_LINE);
    }
}

static void create_dir(void)
{
    char *mesg = "Insert new folder name:> ", str[80];
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, str);
    noecho();
    if (mkdir(str, 0700) == - 1) {
        print_info(strerror(errno), ERR_LINE);
    } else {
        list_everything(ps.active, 0, dim - 2, 1, 1);
        sync_changes();
        print_info("Folder created.", INFO_LINE);
    }
}

static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    return remove(path);
}

static int rmrf(char *path)
{
    if (access(path, W_OK) == 0)
        return nftw(path, recursive_remove, 64, FTW_DEPTH | FTW_PHYS | FTW_MOUNT);
    return -1;
}

static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int i = 0;
    char str[PATH_MAX];
    while (found_searched[i]) {
        i++;
    }
    strcpy(str, strrchr(path, '/'));
    memmove(str, str + 1, strlen(str));
    if (strncmp(str, searched_string, strlen(searched_string)) == 0) {
        found_searched[i] = malloc(sizeof(char) * PATH_MAX);
        strcpy(found_searched[i], path);
        if (typeflag == FTW_D)
            strcat(found_searched[i], "/");
    }
    return 0;
}

static int search_file(char *path)
{
    return nftw(path, recursive_search, 64, FTW_MOUNT | FTW_PHYS);
}

static void search(void)
{
    char *mesg = "Insert filename to be found, at least 3 chars:> ";
    int i = 0, old_size = ps.number_of_files[ps.active];
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, searched_string);
    noecho();
    if (strlen(searched_string) < 3) {
        clear_info(INFO_LINE);
        return;
    }
    search_file(ps.my_cwd[ps.active]);
    if (!found_searched[i]) {
        print_info("No files found.", INFO_LINE);
        return;
    }
    wclear(file_manager[ps.active]);
    wborder(file_manager[ps.active], '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(file_manager[ps.active], 0, 0, "Found file searching %s: ", searched_string);
    wattron(file_manager[ps.active], A_BOLD);
    for (i = 0; (i < dim - 2) && (found_searched[i]); i++)
        mvwprintw(file_manager[ps.active], INITIAL_POSITION + i, 4, "%s", found_searched[i]);
    wattroff(file_manager[ps.active], A_BOLD);
    while (found_searched[i])
        i++;
    if (search_loop(i) == 'q') {
        ps.number_of_files[ps.active] = old_size;
        list_everything(ps.active, 0, dim - 2, 1, 1);
    }
    clear_info(INFO_LINE);
    for (i = 0; found_searched[i]; i++) {
        found_searched[i] = realloc(found_searched[i], 0);
        free(found_searched[i]);
    }
}

static int search_loop(int size)
{
    char *str = NULL, *mesg = "Open file? y to open, n to switch to the folder";
    int c, old_size = ps.number_of_files[ps.active];
    ps.delta[ps.active] = 0;
    ps.current_position[ps.active] = 0;
    ps.number_of_files[ps.active] = size;
    print_info("q to leave search win", INFO_LINE);
    mvwprintw(file_manager[ps.active], INITIAL_POSITION, 1, "->");
    do {
        c = wgetch(file_manager[ps.active]);
        wattron(file_manager[ps.active], A_BOLD);
        switch (c) {
            case KEY_UP:
                scroll_up(found_searched[ps.current_position[ps.active]]);
                break;
            case KEY_DOWN:
                scroll_down(found_searched[ps.current_position[ps.active]]);
                break;
            case 10:
                str = strrchr(found_searched[ps.current_position[ps.active]], '/');
                if ((strlen(str) != 1) && (ask_user(mesg) == 1))
                    manage_file(found_searched[ps.current_position[ps.active]]);
                else {
                    found_searched[ps.current_position[ps.active]][strlen(found_searched[ps.current_position[ps.active]]) - strlen(str)] = '\0';
                    ps.number_of_files[ps.active] = old_size;
                    change_dir(found_searched[ps.current_position[ps.active]]);
                }
                break;
        }
        wattroff(file_manager[ps.active], A_BOLD);
        if ((c == KEY_UP) || (c == KEY_DOWN)) {
            wborder(file_manager[ps.active], '|', '|', '-', '-', '+', '+', '+', '+');
            mvwprintw(file_manager[ps.active], 0, 0, "Found file searching %s: ", searched_string);
        }
    } while ((c != 'q') && (ps.number_of_files[ps.active] == size));
    return c;
}

static void print_support(char *str)
{
    pid_t pid;
    char *mesg = "Do you really want to print this file? y/n:> ";
    if (ask_user(mesg) == 1) {
        if (access("/usr/bin/lpr", F_OK ) != -1) {
            pid = vfork();
            if (pid == 0)
                execl("/usr/bin/lpr", "/usr/bin/lpr", str, NULL);
        } else {
            print_info("You must have cups installed.", ERR_LINE);
        }
    }
}

/* Helper functions */
static int isIso(char *ext)
{
    int i = 0;
    while (*(iso_extensions + i)) {
        if (strcmp(ext, *(iso_extensions + i)) == 0)
            return 1;
        i++;
    }
    return 0;
}

static int file_isCopied(void)
{
    char full_path_current_position[PATH_MAX];
    copied_file_list *tmp = ps.copied_files;
    get_full_path(full_path_current_position);
    while (tmp) {
        if (strcmp(tmp->copied_file, full_path_current_position) == 0) {
            print_info("The file is already selected for copy. Please cancel the copy before.", ERR_LINE);
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

static void get_full_path(char *full_path_current_position)
{
    strcpy(full_path_current_position, ps.my_cwd[ps.active]);
    strcat(full_path_current_position, "/");
    strcat(full_path_current_position,namelist[ps.active][ps.current_position[ps.active]]->d_name);
}

static int ask_user(char *str)
{
    char c;
    echo();
    print_info(str, INFO_LINE);
    c = wgetch(info_win);
    noecho();
    clear_info(INFO_LINE);
    if (c == 'y')
        return 1;
    return 0;
}

/* Quit functions */
static void free_copied_list(copied_file_list *h)
{
    if (h->next)
        free_copied_list(h->next);
    free(h);
}

static void free_everything(void)
{
    int i, j;
    for (j = 0; j < ps.cont; j++) {
        if (namelist[j] != NULL) {
            for (i = ps.number_of_files[j] - 1; i >= 0; i--)
                free(namelist[j][i]);
            free(namelist[j]);
        }
    }
    free(config.editor);
    free(config.iso_mount_point);
    if (ps.copied_files)
        free_copied_list(ps.copied_files);
}


static void quit_func(void)
{
    char *mesg = "A paste job is still running. Do you want to wait for it?(You should!) y/n:> ";
    if ((ps.pasted == - 1) && (ask_user(mesg) == 1))
        pthread_join(th, NULL);
}
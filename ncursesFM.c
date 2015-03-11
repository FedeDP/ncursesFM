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
};

struct vars {
    int current_position[MAX_TABS];
    int number_of_files[MAX_TABS];
    int active;
    int cont;
    int delta[MAX_TABS];
    int stat_active[MAX_TABS];
    char *copied_file;
    char my_cwd[MAX_TABS][PATH_MAX];
};

static void list_everything(int win, int old_dim, int end, int erase, int reset);
static int is_hidden(const struct dirent *current_file);
static void screen_init(void);
static void screen_end(void);
static void main_loop(int *quit, int *cut);
static void change_dir(char *str);
static void switch_hidden(void);
static void manage_file(char *str);
static void open_file(char *str);
static void helper_function(int argc, char *argv[]);
static void init_func(void);
static void iso_mount_service(char *str);
static void new_file(void);
static void remove_file(char *str);
static void my_sort(int win);
static void new_tab(void);
static void delete_tab(void);
static void scroll_down(void);
static void scroll_up(void);
static void scroll_helper_func(int x, int direction);
static void sync_changes(void);
static void colored_folders(int i, int win);
static void copy_file(char *str);
static void paste_file(int **cut);
static void *thread_paste(void *pasted);
static void check_pasted(int **cut);
static void undo_copy(void);
static void rename_file_folders(void);
static void create_dir(void);
static void free_everything(void);
static void print_info(char *str, int i);
static void clear_info(int i);
static void trigger_show_helper_message(void);
static void helper_print(void);
static void show_stat(int init, int end, int win);

static const char *config_file_name = "ncursesFM.conf";
static WINDOW *file_manager[MAX_TABS], *info_win, *helper_win = NULL;
static char pasted_dir[PATH_MAX], copied_dir[PATH_MAX];
static int dim, pasted = 0;
struct dirent **namelist[MAX_TABS] = {NULL, NULL};
static struct conf config = {
    .editor = NULL,
    .show_hidden = 0,
    .iso_mount_point = NULL
};
static struct vars ps = {
    .active = 0,
    .cont = 1,
    .delta = {0, 0},
    .stat_active = {0, 0},
    .copied_file = NULL
};


int main(int argc, char *argv[])
{
    int quit = 0, cut = 0;
    helper_function(argc, argv);
    init_func();
    screen_init();
    getcwd(ps.my_cwd[ps.active], PATH_MAX);
    list_everything(ps.active, 0, dim - 2, 1, 1);
    while (!quit)
        main_loop(&quit, &cut);
    free_everything();
    screen_end();
    return 0;
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
    mvwprintw(file_manager[win], 0, 0, "Current dir: %.*s ", COLS / ps.cont, ps.my_cwd[win]);
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

static void main_loop(int *quit, int *cut)
{
    int c;
    if (ps.copied_file != NULL)
        print_info("A file is waiting to be pasted.", INFO_LINE);
    if (pasted != 0)
        check_pasted(&cut);
    c = wgetch(file_manager[ps.active]);
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
            if ((namelist[ps.active][ps.current_position[ps.active]]->d_type ==  DT_DIR) ||
                (namelist[ps.active][ps.current_position[ps.active]]->d_type ==  DT_LNK ))
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
            remove_file(namelist[ps.active][ps.current_position[ps.active]]->d_name);
            break;
        case 'c': case 'x': // copy file
            if ((namelist[ps.active][ps.current_position[ps.active]]->d_type !=  DT_DIR) && (ps.copied_file == NULL))
                copy_file(namelist[ps.active][ps.current_position[ps.active]]->d_name);
            else if (ps.copied_file != NULL)
                undo_copy();
            if (c == 'x')
                *cut = 1;
            break;
        case 'v': // paste file
            if (ps.copied_file != NULL)
                paste_file(&cut);
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
        case 'q': /* q to exit */
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
    if (strcmp(str + (strlen(str) - 4), ".iso") == 0) {
        if (config.iso_mount_point != NULL)
            iso_mount_service(str);
        else
            print_info("You have to specify an iso mount point in config file.", ERR_LINE);
    } else {
        if ((config.editor != NULL) && (namelist[ps.active][ps.current_position[ps.active]]->d_type == DT_REG))
            open_file(str);
        else
            print_info("You have to specify an editor in config file.", ERR_LINE);
    }
}

static void open_file(char *str)
{
    pid_t pid;
    endwin();
    pid = fork();
    if (pid == 0) { /* child */
        if (execl(config.editor, config.editor, str, NULL) == -1)
            print_info(strerror(errno), ERR_LINE);
    } else {
        waitpid(pid, NULL, 0);
    }
    refresh();
}

static void helper_function(int argc, char *argv[])
{
    if (argc == 1)
        return;
    if (strcmp(argv[1], "-h") != 0)
        printf("Use '-h' to view helper message\n");
    else {
        printf("\tNcurses FM Helper message:\n");
        printf("\t\t* Just use arrow keys to move up and down, and enter to change directory or open a file.\n");
        printf("\t\t* Press 'l' during the program to view a more detailed helper message.\n");
        printf("\tWritten by: Federico Di Pierro: https://github.com/FedeDP.\n.");
    }
    exit(0);
}

static void init_func(void)
{
    config_t cfg;
    const char *str, *str2;
    config_init(&cfg);
    if (!config_read_file(&cfg, config_file_name)) {
        printf("\n%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        exit(1);
    }
    if (config_lookup_string(&cfg, "editor", &str)) {
        config.editor = malloc(strlen(str) * sizeof(char) + 1);
        strcpy(config.editor, str);
    }
    config_lookup_int(&cfg, "show_hidden", &config.show_hidden);
    if (config_lookup_string(&cfg, "iso_mount_point", &str2)) {
        config.iso_mount_point = malloc(strlen(str2) * sizeof(char) + 1);
        strcpy(config.iso_mount_point, str2);
    }
    config_destroy(&cfg);
}

static void iso_mount_service(char *str)
{
    pid_t pid;
    char mount_point[strlen(str) - 4 + strlen(config.iso_mount_point)];
    strcpy(mount_point, config.iso_mount_point);
    strncat(mount_point, str, strlen(str) - 4);
    pid = fork();
    if (pid == 0) {
        if (mkdir(mount_point, ACCESSPERMS) == -1) {
            execl("/usr/bin/fusermount", "/usr/bin/fusermount", "-u", mount_point, NULL);
        } else
            execl("/usr/bin/fuseiso", "/usr/bin/fuseiso", str, mount_point, NULL);
    } else {
        waitpid(pid, NULL, 0);
    }
    rmdir(mount_point);
    list_everything(ps.active, 0, dim - 2, 1, 1);
    sync_changes();
}

static void new_file(void)
{
    FILE *f;
    char *mesg = "Insert new file name:> ", str[80];
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

static void remove_file(char *str)
{
    char mesg[25] = "Are you serious? y/n:> ", c;
    echo();
    print_info(mesg, INFO_LINE);
    c = wgetch(info_win);
    if (c == 'y') {
        if (remove(str) == - 1) {
            print_info(strerror(errno), ERR_LINE);
        } else {
            list_everything(ps.active, 0, dim - 2, 1, 1);
            sync_changes();
            print_info("File/dir removed.", INFO_LINE);
        }
    } else {
        clear_info(INFO_LINE);
    }
    noecho();
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
    wresize(file_manager[ps.active], dim, COLS / 2);
    wborder(file_manager[ps.active], '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(file_manager[ps.active], 0, 0, "Current dir: %.*s ", COLS / ps.cont, ps.my_cwd[ps.active]);
    wrefresh(file_manager[ps.active]);
    ps.active = 1;
    file_manager[ps.active] = subwin(stdscr, dim, COLS / 2, 0, COLS / 2);
    ps.cont++;
    keypad(file_manager[ps.active], TRUE);
    scrollok(file_manager[ps.active], TRUE);
    idlok(file_manager[ps.active], TRUE);
    getcwd(ps.my_cwd[ps.active], PATH_MAX);
    list_everything(ps.active, 0, dim - 2, 1, 1);
}

static void delete_tab(void)
{
    int i;
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
    mvwprintw(file_manager[ps.active], 0, 0, "Current dir: %.*s ", COLS / ps.cont, ps.my_cwd[ps.active]);
    ps.cont--;
}

static void scroll_down(void)
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
        list_everything(ps.active, ps.current_position[ps.active], 1, 0, 0);
    } else {
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active], 1, "  ");
        mvwprintw(file_manager[ps.active], ps.current_position[ps.active] - ps.delta[ps.active] + INITIAL_POSITION, 1, "->");
    }
}

static void scroll_up(void)
{
    ps.current_position[ps.active]--;
    if (ps.current_position[ps.active] < 0) {
        ps.current_position[ps.active]++;
        return;
    }
    if (ps.current_position[ps.active] < ps.delta[ps.active]) {
        scroll_helper_func(INITIAL_POSITION, - 1);
        ps.delta[ps.active]--;
        list_everything(ps.active, ps.delta[ps.active], 1, 0, 0);
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
        if ((ps.copied_file != NULL) && (strncmp(ps.copied_file, ps.my_cwd[1 - ps.active], strlen(ps.my_cwd[1 - ps.active])) == 0))
            list_everything(1 - ps.active, 0, dim - 2, 1, 1);
    }
}

static void colored_folders(int i, int win)
{
    struct stat file_stat;
    wattron(file_manager[win], A_BOLD);
    if ((namelist[win][i]->d_type == DT_DIR) || (namelist[win][i]->d_type == DT_LNK)) {
        wattron(file_manager[win], COLOR_PAIR(1));
        if (namelist[win][i]->d_type == DT_LNK)
            wattron(file_manager[win], COLOR_PAIR(3));
    } else if (namelist[win][i]->d_type == DT_REG) {
        stat(namelist[win][i]->d_name, &file_stat);
        if (file_stat.st_mode & S_IXUSR)
            wattron(file_manager[win], COLOR_PAIR(2));
    }
}

static void copy_file(char *str)
{
    ps.copied_file = malloc(strlen(str) + strlen(ps.my_cwd[ps.active]) + 1);
    strcpy(ps.copied_file, ps.my_cwd[ps.active]);
    strcat(ps.copied_file, "/");
    strcat(ps.copied_file, str);
    strcpy(copied_dir, ps.my_cwd[ps.active]);
}

static void paste_file(int **cut)
{
    pthread_t th;
    struct stat file_stat_copied, file_stat_pasted;
    strcpy(pasted_dir, ps.my_cwd[ps.active]);
    stat(ps.copied_file, &file_stat_copied);
    stat(pasted_dir, &file_stat_pasted);
    if ((file_stat_copied.st_dev == file_stat_pasted.st_dev) && (**cut == 1)) {
        strcat(pasted_dir, strrchr(ps.copied_file, '/'));
        if (rename(ps.copied_file, pasted_dir) == - 1) {
            print_info(strerror(errno), ERR_LINE);
        } else {
            **cut = 0;
            list_everything(ps.active, 0, dim - 2, 1, 1);
            sync_changes();
            print_info("File renamed.", INFO_LINE);
            free(ps.copied_file);
            ps.copied_file = NULL;
            memset(pasted_dir, 0, strlen(pasted_dir));
            pasted = 0;
        }
    } else {
        pthread_create(&th, NULL, thread_paste, &pasted);
    }
}

static void *thread_paste(void *pasted)
{
    FILE *source_fp, *dest_fp;
    char str[80], buffer[512];
    int n = 1;
    source_fp = fopen(ps.copied_file, "r");
    strcpy(str, ps.my_cwd[ps.active]);
    strcat(str, strrchr(ps.copied_file, '/'));
    if (access(str, F_OK) != - 1) {
        fclose(source_fp);
        *((int *)pasted) = -1;
        return NULL;
    } else {
        dest_fp = fopen(str, "w");
        while (n != EOF) {
            n = fread(buffer, 1, 512, source_fp);
            if (n == 0)
                break;
            fwrite(buffer, 1, n, dest_fp);
        }
    }
    *((int *)pasted) = 1;
    fclose(source_fp);
    fclose(dest_fp);
    return NULL;
}

static void check_pasted(int **cut)
{
    int i;
    switch (pasted) {
        case 1:
            if (**cut == 1) {
                remove(ps.copied_file);
                **cut = 0;
                for (i = 0; i < ps.cont; i++) {
                    if (strcmp(copied_dir, ps.my_cwd[i]) == 0)
                        list_everything(i, 0, dim - 2, 1, 1);
                }
            }
            for (i = 0; i < ps.cont; i++) {
                if (strcmp(pasted_dir, ps.my_cwd[i]) == 0)
                    list_everything(i, 0, dim - 2, 1, 1);
            }
            free(ps.copied_file);
            ps.copied_file = NULL;
            memset(pasted_dir, 0, strlen(pasted_dir));
            print_info("File copied.", INFO_LINE);
            pasted = 0;
            break;
        case -1:
            print_info("Cannot copy here. Check user permissions.", ERR_LINE);
            pasted = 0;
            break;
    }
}

static void undo_copy(void)
{
    free(ps.copied_file);
    ps.copied_file = NULL;
    print_info("Copy file canceled.", INFO_LINE);
}

static void rename_file_folders(void)
{
    char *mesg = "Insert new name:> ", str[80];
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, str);
    if (rename(namelist[ps.active][ps.current_position[ps.active]]->d_name, str) == - 1) {
        print_info(strerror(errno), ERR_LINE);
    } else {
        list_everything(ps.active, 0, dim - 2, 1, 1);
        sync_changes();
        print_info("File renamed.", INFO_LINE);
    }
    noecho();
}

static void create_dir(void)
{
    char *mesg = "Insert new folder name:> ", str[80];
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, str);
    if (mkdir(str, 0700) == - 1) {
        print_info(strerror(errno), ERR_LINE);
    } else {
        list_everything(ps.active, 0, dim - 2, 1, 1);
        sync_changes();
        print_info("Folder created.", INFO_LINE);
    }
    noecho();
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
    free(ps.copied_file);
}

static void print_info(char *str, int i)
{
    clear_info(i);
    mvwprintw(info_win, i, 1, str);
    wrefresh(info_win);
}

static void clear_info(int i)
{
    wmove(info_win, i, 0);
    wclrtoeol(info_win);
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
            mvwprintw(file_manager[i], 0, 0, "Current dir: %.*s ", COLS / ps.cont, ps.my_cwd[i]);
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
    wprintw(helper_win, " * Enter to surf between folders (follows ls colors) or to open files with $editor var.\n");
    wprintw(helper_win, " * Enter will eventually mount your ISO files in $path.\n");
    wprintw(helper_win, " * You must have fuseiso installed. To unmount, simply press again enter on the same iso file.\n");
    wprintw(helper_win, " * Press h to trigger the showing of hide files.\n");
    wprintw(helper_win, " * c to copy, p to paste, and x to cut a file. c again to remove from memory previously copied (not yet pasted) file.\n");
    wprintw(helper_win, " * s to see stat about files in current folder (Sizes and permissions).\n");
    wprintw(helper_win, " * o to rename current file/dir; d to create new dir.\n");
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
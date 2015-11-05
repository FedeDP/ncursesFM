#include "../inc/fm_functions.h"

#ifdef LIBX11_PRESENT
static void xdg_open(const char *str, float size);
#endif
static void open_file(const char *str, float size);
static int new_file(const char *name);
static int new_dir(const char *name);
static int rename_file_folders(const char *name);
static void cpr(file_list *tmp);
static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void rmrf(const char *path);

static int num_files;
#ifdef SYSTEMD_PRESENT
static const char *iso_ext[] = {".iso", ".nrg", ".bin", ".mdf", ".img"};
static const char *pkg_ext[] = {".pkg.tar.xz", ".deb", ".rpm"};
#endif
static struct timeval timer;
static char fast_browse_str[NAME_MAX];
static int (*const short_func[SHORT_FILE_OPERATIONS])(const char *) = {
    new_file, new_dir, rename_file_folders
};

void change_dir(const char *str) {
    if (chdir(str) != -1) {
        getcwd(ps[active].my_cwd, PATH_MAX);
        sprintf(ps[active].title, "Current: %s", ps[active].my_cwd);
        tab_refresh(active);
    } else {
        print_info(strerror(errno), ERR_LINE);
    }
}

void switch_hidden(void) {
    int i;

    config.show_hidden = !config.show_hidden;
    for (i = 0; i < cont; i++) {
        tab_refresh(i);
    }
}

int is_ext(const char *filename, const char *ext[], int size) {
    int i = 0, len = strlen(filename);
    
    if (strrchr(filename, '.')) {
        while (i < size) {
            if (strcmp(filename + len - strlen(ext[i]), ext[i]) == 0) {
                return 1;
            }
            i++;
        }
    }
    return 0;
}

/*
 * Check if it is an iso, then try to mount it.
 * If it is a package, ask user to mount it.
 * If it is an archive, initialize a thread job to extract it.
 * if compiled with X11 support, and xdg-open is found, open the file,
 * else open the file with $config.editor.
 */
void manage_file(const char *str, float size) {
    char c;
    
#ifdef SYSTEMD_PRESENT
    if (is_ext(str, iso_ext, NUM(iso_ext))) {
        return isomount(str);
    }
    if (is_ext(str, pkg_ext, NUM(pkg_ext))) {
        ask_user(pkg_quest, &c, 1, 'n');
        if (c == 'y') {
            pthread_create(&install_th, NULL, install_package, (void *)str);
            return;
        }
        return;
    }
#endif
    if (is_ext(str, arch_ext, NUM(arch_ext))) {
        ask_user(extr_question, &c, 1, 'y');
        if (c == 'y') {
            return init_thread(EXTRACTOR_TH, try_extractor);
        }
        return;
    }
#ifdef LIBX11_PRESENT
    if (access("/usr/bin/xdg-open", X_OK) != -1) {
        return xdg_open(str, size);
    }
#endif
    return open_file(str, size);
}

/*
 * If we're on a X screen, open the file with xdg-open, and redirect its output to /dev/null
 * not to dirty my ncurses screen.
 */
#ifdef LIBX11_PRESENT
static void xdg_open(const char *str, float size) {
    pid_t pid;
    Display* display = XOpenDisplay(NULL);

    if (display) {
        XCloseDisplay(display);
        pid = vfork();
        if (pid == 0) {
            int fd = open("/dev/null",O_WRONLY);
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);
            execl("/usr/bin/xdg-open", "/usr/bin/xdg-open", str, NULL);
        }
    } else {
        return open_file(str, size);
    }
}
#endif

static void open_file(const char *str, float size) {
    pid_t pid;
    char c;

    if (size > BIG_FILE_THRESHOLD) { // 5 Mb
        ask_user(big_file, &c, 1, 'y');
        if (c == 'n') {
            return;
        }
    }
    if (strlen(config.editor)) {
        endwin();
        pid = vfork();
        if (pid == 0) {
            execl(config.editor, config.editor, str, NULL);
        } else {
            waitpid(pid, NULL, 0);
        }
        refresh();
    } else {
        print_info(editor_missing, ERR_LINE);
    }
}

void fast_file_operations(const int index) {
    char new_name[NAME_MAX] = {0};
    const char *str = short_fail_msg[index];
    int line = ERR_LINE, i;

    ask_user(ask_name, new_name, NAME_MAX, 0);
    if (!strlen(new_name)) {
        return;
    }
    if (short_func[index](new_name) == 0) {
        str = short_msg[index];
        line = INFO_LINE;
        for (i = 0; i < cont; i++) {
            if (strcmp(ps[i].my_cwd, ps[active].my_cwd) == 0) {
                tab_refresh(i);
            }
        }
    }
    print_info(str, line);
}

int new_file(const char *name) {
    int fd;

    fd = open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd != -1) {
        close(fd);
        return 0;
    }
    return -1;
}

static int new_dir(const char *name) {
    return mkdir(name, 0700);
}

static int rename_file_folders(const char *name) {
    return rename(ps[active].nl[ps[active].curr_pos], name);
}

int remove_file(void) {
    int ok = 0;
    file_list *tmp = thread_h->selected_files;

    while (tmp) {
        if (access(tmp->name, W_OK) == 0) {
            ok++;
            rmrf(tmp->name);
        }
        tmp = tmp->next;
    }
    return (ok ? 0 : -1);
}

/*
 * If there are no selected files, or the file user selected wasn't already selected,
 * add this file to select list.
 */
void manage_space_press(const char *str) {
    const char *s;

    if ((!selected) || (remove_from_list(str) == 0)) {
        selected = select_file(selected, str);
        s = file_sel[0];
    } else {
        ((selected) ? (s = file_sel[1]) : (s = file_sel[2]));
    }
    print_info(s, INFO_LINE);
}

int paste_file(void) {
    char copied_file_dir[PATH_MAX], *str;
    int len;
    file_list *tmp = NULL;

    for (tmp = thread_h->selected_files; tmp; tmp = tmp->next) {
        strcpy(copied_file_dir, tmp->name);
        str = strrchr(tmp->name, '/');
        len = strlen(tmp->name) - strlen(str);
        copied_file_dir[len] = '\0';
        if (strcmp(thread_h->full_path, copied_file_dir) != 0) {
            cpr(tmp);
        }
    }
    return 0;
}

int move_file(void) {
    char pasted_file[PATH_MAX], copied_file_dir[PATH_MAX], *str;
    struct stat file_stat_copied, file_stat_pasted;
    file_list *tmp = NULL;
    int len;

    lstat(thread_h->full_path, &file_stat_pasted);
    for (tmp = thread_h->selected_files; tmp; tmp = tmp->next) {
        strcpy(copied_file_dir, tmp->name);
        str = strrchr(tmp->name, '/');
        len = strlen(tmp->name) - strlen(str);
        copied_file_dir[len] = '\0';
        if (strcmp(thread_h->full_path, copied_file_dir) != 0) {
            lstat(copied_file_dir, &file_stat_copied);
            if (file_stat_copied.st_dev == file_stat_pasted.st_dev) { // if on the same fs, just rename the file
                sprintf(pasted_file, "%s%s", thread_h->full_path, strrchr(tmp->name, '/'));
                if (rename(tmp->name, pasted_file) == - 1) {
                    print_info(strerror(errno), ERR_LINE);
                }
            } else { // copy file and remove original file
                cpr(tmp);
                rmrf(tmp->name);
            }
        }
    }
    return 0;
}

static void cpr(file_list *tmp) {
    char *str;
    
    str = strrchr(tmp->name, '/');
    distance_from_root = strlen(tmp->name) - strlen(str);
    nftw(tmp->name, recursive_copy, 64, FTW_MOUNT | FTW_PHYS);
}

static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int buff[BUFF_SIZE];
    int len, fd_to, fd_from;
    char pasted_file[PATH_MAX];

    sprintf(pasted_file, "%s%s", thread_h->full_path, path + distance_from_root);
    if (typeflag == FTW_D) {
        mkdir(pasted_file, sb->st_mode);
    } else {
        fd_to = open(pasted_file, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC, sb->st_mode);
        fd_from = open(path, O_RDONLY);
        if ((fd_to != -1) && (fd_from != -1)) {
            len = read(fd_from, buff, sizeof(buff));
            while (len > 0) {
                write(fd_to, buff, len);
                len = read(fd_from, buff, sizeof(buff));
            }
            close(fd_to);
            close(fd_from);
        }
    }
    return 0;
}

static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    return remove(path);
}

static void rmrf(const char *path) {
    nftw(path, recursive_remove, 64, FTW_DEPTH | FTW_PHYS | FTW_MOUNT);
}

void change_tab(void) {
    active = (!active + cont) % MAX_TABS;
    chdir(ps[active].my_cwd);
}

void fast_browse(int c) {
    int i = 1, found = 0, end = ps[active].number_of_files;
    int len;
    char *str;
    uint64_t diff = (MILLION * timer.tv_sec) + timer.tv_usec;
    void (*f)(void);

    gettimeofday(&timer, NULL);
    diff = MILLION * (timer.tv_sec) + timer.tv_usec - diff;
    if ((diff < FAST_BROWSE_THRESHOLD) && (num_files)) { // 0,5s
        i = ps[active].curr_pos;
        end = i + num_files;
    } else {
        memset(fast_browse_str, 0, strlen(fast_browse_str));
    }
    sprintf(fast_browse_str + strlen(fast_browse_str), "%c", c);
    print_info(fast_browse_str, INFO_LINE);
    for (num_files = 0; i < end; i++) {
        str = strrchr(ps[active].nl[i], '/') + 1;
        len = strlen(fast_browse_str);
        if (strncmp(fast_browse_str, str, len) == 0) {
            if (!found) {
                found = 1;
                if (i < ps[active].curr_pos) {
                    f = scroll_up;
                } else {
                    f = scroll_down;
                }
                while (ps[active].curr_pos != i) {
                    f();
                }
            }
            num_files++;
        }
    }
    if ((!found) && (strlen(fast_browse_str) > 1)) {
        memset(fast_browse_str, 0, strlen(fast_browse_str));
        print_info(fast_browse_str, INFO_LINE);
    }
}
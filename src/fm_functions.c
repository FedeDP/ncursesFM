#include "../inc/fm_functions.h"

static void xdg_open(const char *str, float size);
static void open_file(const char *str, float size);
static int new_file(const char *name);
static int new_dir(const char *name);
static int rename_file_folders(const char *name);
static void cpr(file_list *tmp);
static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)
static loff_t copy_file_range(int fd_in, loff_t *off_in, int fd_out,
                              loff_t *off_out, size_t len, unsigned int flags);
#endif
static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void rmrf(const char *path);

#ifdef SYSTEMD_PRESENT
static const char *iso_ext[] = {".iso", ".nrg", ".bin", ".mdf", ".img"};
static const char *pkg_ext[] = {".pkg.tar.xz", ".deb", ".rpm"};
#endif
static struct timeval timer;
static char fast_browse_str[NAME_MAX + 1];
static int distance_from_root;
static int (*const short_func[SHORT_FILE_OPERATIONS])(const char *) = {
    new_file, new_dir, rename_file_folders
};

int change_dir(const char *str, int win) {
    int ret;
    const int event_mask = IN_MODIFY | IN_ATTRIB | IN_CREATE | IN_DELETE | IN_MOVE;
    
    if (chdir(str) != -1) {
        getcwd(ps[win].my_cwd, PATH_MAX);
        sprintf(ps[win].title, "%s", ps[win].my_cwd);
        tab_refresh(win);
        inotify_rm_watch(ps[win].inot.fd, ps[win].inot.wd);
        ps[win].inot.wd = inotify_add_watch(ps[win].inot.fd, ps[win].my_cwd, event_mask);
        ret = 0;
    } else {
        print_info(strerror(errno), ERR_LINE);
        ret = -1;
    }
    return ret;
}

void change_tab(void) {
    active = !active;
    chdir(ps[active].my_cwd);
    update_colors();
}

void switch_hidden(void) {
    ps[active].show_hidden = !ps[active].show_hidden;
    save_old_pos(active);
    tab_refresh(active);
}

/*
 * Check if filename has "." in it (otherwise surely it has not extension)
 * Then for each extension in *ext[], check if last strlen(ext[i]) chars of filename are 
 * equals to ext[i].
 */
int is_ext(const char *filename, const char *ext[], int size) {
    int i = 0, len = strlen(filename);
    
    if (strrchr(filename, '.')) {
        while (i < size) {
            if (!strcmp(filename + len - strlen(ext[i]), ext[i])) {
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
 * else open the file with config.editor.
 */
void manage_file(const char *str, float size) {
    char c;
    
#ifdef SYSTEMD_PRESENT
    if (is_ext(str, iso_ext, NUM(iso_ext))) {
        isomount(str);
        return;
    }
    if (is_ext(str, pkg_ext, NUM(pkg_ext))) {
        print_info(package_warn, INFO_LINE);
        ask_user(pkg_quest, &c, 1);
        print_info("", INFO_LINE);
        if (!quit && c == 'y') {
            pthread_create(&install_th, NULL, install_package, (void *)str);
        }
        return;
    }
#endif
    if (is_ext(str, arch_ext, NUM(arch_ext))) {
        ask_user(extr_question, &c, 1);
        if (!quit && c != 'n') {
            init_thread(EXTRACTOR_TH, try_extractor);
        }
        return;
    }
    if (access("/usr/bin/xdg-open", X_OK) == 0) {
        xdg_open(str, size);
    } else {
        open_file(str, size);
    }
}

/*
 * If we're on a X screen, open the file with xdg-open and redirect its output to /dev/null
 * not to make my ncurses screen dirty.
 */
static void xdg_open(const char *str, float size) {
    if (has_X) {
        pid_t pid = vfork();
        if (pid == 0) {
            int fd = open("/dev/null",O_WRONLY);
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);
            execl("/usr/bin/xdg-open", "/usr/bin/xdg-open", str, (char *) 0);
        }
    } else {
        open_file(str, size);
    }
}

/*
 * If file size is > than 5 Mb, asks user if he really wants to open the file 
 * (maybe he pressed enter on a avi video for example), then, if config.editor is set,
 * opens the file with it.
 */
static void open_file(const char *str, float size) {
    char c;

    if (size > BIG_FILE_THRESHOLD) { // 5 Mb
        ask_user(big_file, &c, 1);
        if (quit || c != 'y') {
            return;
        }
    }
    if (strlen(config.editor)) {
        endwin();
        pid_t pid = vfork();
        if (pid == 0) {
            execl(config.editor, config.editor, str, (char *) 0);
        } else {
            waitpid(pid, NULL, 0);
            // trigger a fake resize to restore previous win state
            // understand why this is needed now (it wasn't needed some time ago as
            // a refresh() was, and should be, enough)
//             refresh();
            ungetch(KEY_RESIZE);

        }
    } else {
        print_info(editor_missing, ERR_LINE);
    }
}

/*
 * Ask user for "new_name" var to be passed to short_func[],
 * then calls short_func[index]; only refresh UI if the call was successful.
 * Notifies user.
 */
void fast_file_operations(const int index) {
    char new_name[NAME_MAX + 1];

    ask_user(ask_name, new_name, NAME_MAX);
    if (quit || !strlen(new_name)) {
        return;
    }
    int r = short_func[index](new_name);
    if (r == 0) {
        print_info(short_msg[index], INFO_LINE);
    } else {
        print_info(strerror(-r), ERR_LINE);
    }
}

int new_file(const char *name) {
    int fd;

    fd = open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd != -1) {
        close(fd);
        return 0;
    }
    return fd;
}

static int new_dir(const char *name) {
    return mkdir(name, 0700);
}

static int rename_file_folders(const char *name) {
    return rename(ps[active].nl[ps[active].curr_pos], name);
}

/*
 * For each file to be removed, checks if we have write perm on it, then
 * remove it.
 * If at least 1 file was removed, the call is considered successful.
 */
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
 * If there are no selected files, or the file user selected wasn't already selected (remove_from_list() returns 0),
 * add this file to select list.
 */
void manage_space_press(const char *str) {
    const char *s;
    char c;

    if ((!selected) || (remove_from_list(str) == 0)) {
        selected = select_file(selected, str);
        s = file_sel[0];
        c = '*';
    } else {
        c = ' ';
        ((selected) ? (s = file_sel[1]) : (s = file_sel[2]));
    }
    print_info(s, INFO_LINE);
    highlight_selected(ps[active].curr_pos, c);
}

/*
 * For each file being paste, it performs a check: 
 * it checks if file is being pasted in the same dir
 * from where it was copied. If it is the case, it does not copy it.
 */
int paste_file(void) {
    char *copied_file_dir, path[PATH_MAX + 1];

    for (file_list *tmp = thread_h->selected_files; tmp; tmp = tmp->next) {
        strcpy(path, tmp->name);
        copied_file_dir = dirname(path);
        if (strcmp(thread_h->full_path, copied_file_dir)) {
            cpr(tmp);
        }
    }
    return 0;
}

/*
 * Same check as paste_file func plus:
 * it checks if copied file dir and directory where the file is being moved
 * are on the same FS; if it is the case, it only renames it.
 * Else, the function has to copy it and rm copied file.
 */
int move_file(void) {
    char pasted_file[PATH_MAX + 1], *copied_file_dir, path[PATH_MAX + 1];
    struct stat file_stat_copied, file_stat_pasted;

    lstat(thread_h->full_path, &file_stat_pasted);
    for (file_list *tmp = thread_h->selected_files; tmp; tmp = tmp->next) {
        strcpy(path, tmp->name);
        copied_file_dir = dirname(path);
        if (strcmp(thread_h->full_path, copied_file_dir)) {
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

/*
 * It calculates the "distance_from_root" of the current file, where root is 
 * the directory from where it is being copied (eg: copying /home/me/Scripts/ -> root is /home/me/).
 * Then calls nftw with recursive_copy.
 * distance_from_root is needed to make pasted_file relative to thread_h->full_path (ie: the folder where we're pasting).
 * In fact pasted_file will be "thread_h->full_path/(path + distance_from_root)".
 * If path is /home/me/Scripts/foo, and root is the same as above (/home/me/), in the pasted folder we'll have:
 * 1) /path/to/pasted/folder/Scripts/,
 * 2) /path/to/pasted/folder/Scripts/me, that is exactly what we wanted.
 */
static void cpr(file_list *tmp) {
    char path[PATH_MAX + 1];
    
    strcpy(path, tmp->name);
    distance_from_root = strlen(dirname(path));
    nftw(tmp->name, recursive_copy, 64, FTW_MOUNT | FTW_PHYS);
}

static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int len, fd_to, fd_from, ret = 0;
    char pasted_file[PATH_MAX + 1];

    sprintf(pasted_file, "%s%s", thread_h->full_path, path + distance_from_root);
    if (typeflag == FTW_D) {
        mkdir(pasted_file, sb->st_mode);
    } else {
        fd_to = open(pasted_file, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC, sb->st_mode);
        fd_from = open(path, O_RDONLY);
        if ((fd_to != -1) && (fd_from != -1)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)  // if linux >= 4.5 let's use copy_file_range
            struct stat stat;
            
            fstat(fd_from, &stat);
            len = stat.st_size;
            do {
                ret = copy_file_range(fd_from, NULL, fd_to, NULL, len, 0);
                if (ret == -1) {
                    ret = -1;
                    break;
                }
                len -= ret;
            } while (len > 0);
#else
            int buff[BUFF_SIZE];
            len = read(fd_from, buff, sizeof(buff));
            while (len > 0) {
                if (write(fd_to, buff, len) != len) {
                    ret = -1;
                    break;
                }
                len = read(fd_from, buff, sizeof(buff));
            }
#endif
        }
        close(fd_to);
        close(fd_from);
    }
    return (ret == -1) ? ret : 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)
static loff_t copy_file_range(int fd_in, loff_t *off_in, int fd_out,
                                          loff_t *off_out, size_t len, unsigned int flags)
{
    return syscall(__NR_copy_file_range, fd_in, off_in, fd_out,
                   off_out, len, flags);
}
#endif

static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    return remove(path);
}
/*
 * Calls recursive_remove starting from the depth: so this func will remove
 * files and folders from the bottom -> eg: removing /home/me/Scripts, where scripts has 
 * {/foo/bar, /acme}, will remove "/acme", then "/foo/bar", and lastly "/foo/".
 * It is needed to assure that we remove directories when they're already empty.
 */
static void rmrf(const char *path) {
    nftw(path, recursive_remove, 64, FTW_DEPTH | FTW_PHYS | FTW_MOUNT);
}

/*
 * It calculates the time diff since previous call. If it is lower than 0,5s,
 * will start from last char of fast_browse_str, otherwise it will start from scratch.
 * For every file in current dir, checks if fast_browse_str and current file name are equals for
 * at least strlen(fast_browse_str chars). Then moves the cursor to the right name.
 * If nothing was found, deletes fast_browse_str.
 */
void fast_browse(int c) {
    int i = 0;
    uint64_t diff = (MILLION * timer.tv_sec) + timer.tv_usec;

    gettimeofday(&timer, NULL);
    diff = MILLION * (timer.tv_sec) + timer.tv_usec - diff;
    if ((diff < FAST_BROWSE_THRESHOLD) && (strlen(fast_browse_str))) { // 0,5s
        i = ps[active].curr_pos;
    } else {
        memset(fast_browse_str, 0, strlen(fast_browse_str));
    }
    sprintf(fast_browse_str + strlen(fast_browse_str), "%c", c);
    print_info(fast_browse_str, INFO_LINE);
    if (!move_cursor_to_file(i, fast_browse_str, active)) {
        memset(fast_browse_str, 0, strlen(fast_browse_str));
    }
}

int move_cursor_to_file(int i, const char *filename, int win) {
    char *str;
    void (*f)(int);
    int len = strlen(filename);
    
    for (; i < ps[win].number_of_files; i++) {
        str = strrchr(ps[win].nl[i], '/') + 1;
        if (strncmp(filename, str, len) == 0) {
            if (i != ps[win].curr_pos) {
                if (i < ps[win].curr_pos) {
                    f = scroll_up;
                } else {
                    f = scroll_down;
                }
                while (ps[win].curr_pos != i) {
                    f(win);
                }
            }
            return 1;
        }
    }
    return 0;
}

void save_old_pos(int win) {
    char *str;
    
    str = strrchr(ps[win].nl[ps[win].curr_pos], '/') + 1;
    strcpy(ps[win].old_file, str);
}

#include "../inc/fm.h"

static void xdg_open(const char *str);
static void open_file(const char *str);
static int new_file(const char *name);
static int new_dir(const char *name);
static int rename_file_folders(const char *name);
static void select_file(const char *str);
static void select_all(void);
static void deselect_all(void);
static void cpr(const char *tmp);
static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)
static loff_t copy_file_range(int fd_in, loff_t *off_in, int fd_out,
                              loff_t *off_out, size_t len, unsigned int flags);
#endif
static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void rmrf(const char *path);

#ifdef SYSTEMD_PRESENT
static const char *pkg_ext[] = {".pkg.tar.xz", ".deb", ".rpm"};
#endif
static int distance_from_root, is_selecting;
static int (*const short_func[SHORT_FILE_OPERATIONS])(const char *) = {
    new_file, new_dir, rename_file_folders
};

int change_dir(const char *str, int win) {
    int ret = 0;
    const int event_mask = IN_MODIFY | IN_ATTRIB | IN_CREATE | IN_DELETE | IN_MOVE;
    
    if (chdir(str) != -1) {
        getcwd(ps[win].my_cwd, PATH_MAX);
        strncpy(ps[win].title, ps[win].my_cwd, PATH_MAX);
        tab_refresh(win);
        inotify_rm_watch(ps[win].inot.fd, ps[win].inot.wd);
        ps[win].inot.wd = inotify_add_watch(ps[win].inot.fd, ps[win].my_cwd, event_mask);
        // reset selecting status (double space to select all)
        // when changing dir
        is_selecting = 0;
    } else {
        print_info(strerror(errno), ERR_LINE);
        ret = -1;
    }
    // force process dir to be active tab's one
    if (win != active) {
        chdir(ps[active].my_cwd);
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
 * Check if it is an iso, then try to mount it.
 * If it is a package, ask user to mount it.
 * If it is an archive, initialize a thread job to extract it.
 * if compiled with X11 support, and xdg-open is found, open the file,
 * else open the file with config.editor.
 */
void manage_file(const char *str) {
#ifdef SYSTEMD_PRESENT
    if (get_mimetype(str, "iso9660")) {
        isomount(str);
        return;
    }
    if (is_ext(str, pkg_ext, NUM(pkg_ext))) {
        char c;
        if (config.safe != UNSAFE) {
            print_info(_(package_warn), INFO_LINE);
            ask_user(_(pkg_quest), &c, 1);
            print_info("", INFO_LINE);
            if (c != _(yes)[0]) {
                return;
            }
        }
        pthread_create(&install_th, NULL, install_package, (void *)str);
        return;
    }
#endif
    if (!access("/usr/bin/xdg-open", X_OK) && has_X) {
        xdg_open(str);
    } else {
        open_file(str);
    }
}

/*
 * If we're on a X screen, open the file with xdg-open and redirect its output to /dev/null
 * not to make my ncurses screen dirty.
 */
static void xdg_open(const char *str) {
    if (vfork() == 0) {
        // redirect stdout and stderr to null
        int fd = open("/dev/null",O_WRONLY);
        dup2(fd, 1);
        dup2(fd, 2);
        close(fd);
        execl("/usr/bin/xdg-open", "/usr/bin/xdg-open", str, (char *) 0);
    }
}

/*
 * If file size is > than 5 Mb, asks user if he really wants to open the file 
 * (maybe he pressed enter on a avi video for example), then, if config.editor is set,
 * opens the file with it.
 */
static void open_file(const char *str) {
    if ((!get_mimetype(str, "text/")) && (!get_mimetype(str, "x-empty"))) {
        return;
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
        print_info(_(editor_missing), ERR_LINE);
    }
}

/*
 * Ask user for "new_name" var to be passed to short_func[],
 * then calls short_func[index]; only refresh UI if the call was successful.
 * Notifies user.
 */
void fast_file_operations(const int index) {
    char new_name[NAME_MAX + 1] = {0};

    ask_user(_(ask_name), new_name, NAME_MAX);
    if (!strlen(new_name) || new_name[0] == 27) {
        return;
    }
    int r = short_func[index](new_name);
    if (r == 0) {
        print_info(_(short_msg[index]), INFO_LINE);
    } else {
        print_info(strerror(-r), ERR_LINE);
    }
}

int new_file(const char *name) {
    int fd = open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
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

    for (int i = 0; i < thread_h->num_selected; i++) {
        if (access(thread_h->selected_files[i], W_OK) == 0) {
            ok++;
            rmrf(thread_h->selected_files[i]);
        }
    }
    return (ok ? 0 : -1);
}

/*
 * If there are no selected files, or the file user selected wasn't already selected (remove_from_list() returns 0),
 * add this file to select list.
 */
void manage_space_press(const char *str) {
    int idx;
    char c;
    int i = is_present(str, selected, num_selected, -1, 0);

    if (i == -1) {
        select_file(str);
        idx = 0;
        c = '*';
    } else {
        selected = remove_from_list(&num_selected, selected, i);
        c = ' ';
        num_selected ? (idx = 1) : (idx = 2);
    }
    print_info(_(file_sel[idx]), INFO_LINE);
    highlight_selected(str, c, active);
    if (!strcmp(ps[active].my_cwd, ps[!active].my_cwd)) {
        highlight_selected(str, c, !active);
    }
    update_special_mode(num_selected, selected, selected_);
}

static void select_file(const char *str) {
    selected = safe_realloc(++num_selected, selected);
    if (quit) {
        return;
    }
    memset(selected[num_selected - 1], 0, PATH_MAX + 1);
    strncpy(selected[num_selected - 1], str, PATH_MAX);
}

void manage_all_space_press(void) {
    int idx;
    
    is_selecting = !is_selecting;
    if (is_selecting) {
        select_all();
        idx = 3;
    } else {
        deselect_all();
        selected ? (idx = 4) : (idx = 5);
    }
    print_info(_(file_sel[idx]), INFO_LINE);
    update_special_mode(num_selected, selected, selected_);
}

static void select_all(void) {
    for (int i = 0; i < ps[active].number_of_files; i++) {
        if (strcmp(strrchr(ps[active].nl[i], '/') + 1, "..")) {
            if (is_present(ps[active].nl[i], selected, num_selected, -1, 0) != -1) {
                continue;
            }
            select_file(ps[active].nl[i]);
            highlight_selected(ps[active].nl[i], '*', active);
            if (!strcmp(ps[active].my_cwd, ps[!active].my_cwd)) {
                highlight_selected(ps[active].nl[i], '*', !active);
            }
        }
    }
}

static void deselect_all(void) {
    for (int i = 0; i < ps[active].number_of_files; i++) {
        int j = is_present(ps[active].nl[i], selected, num_selected, -1, 0);
        if (j != -1) {
            selected = remove_from_list(&num_selected, selected, j);
            highlight_selected(ps[active].nl[i], ' ', active);
            if (!strcmp(ps[active].my_cwd, ps[!active].my_cwd)) {
                highlight_selected(ps[active].nl[i], ' ', !active);
            }
        }
    }
}

void remove_selected(void) {
    int idx;
    
    if (!strcmp(ps[active].my_cwd, ps[!active].my_cwd)) {
        highlight_selected(selected[ps[active].curr_pos], ' ', !active);
    }
    selected = remove_from_list(&num_selected, selected, ps[active].curr_pos);
    if (num_selected) {
        idx = 1;
    } else {
        idx = 2;
    }
    update_special_mode(num_selected, selected, selected_);
    print_info(_(file_sel[idx]), INFO_LINE);
}

void remove_all_selected(void) {
    for (int i = 0; i < num_selected; i++) {
        if (cont == 2) {
            highlight_selected(selected[i], ' ', !active);
        }
    }
    free(selected);
    selected = NULL;
    num_selected = 0;
    update_special_mode(num_selected, selected, selected_);
    print_info(_(selected_cleared), INFO_LINE);
}

void show_selected(void) {
    if (num_selected) {
        show_special_tab(num_selected, selected, selected_mode_str, selected_);
    } else {
        print_info(_(no_selected_files), INFO_LINE);
    }
}

void free_selected(void) {
    if (selected) {
        free(selected);
    }
}

/*
 * For each file being pasted, it performs a check: 
 * it checks if file is being pasted in the same dir
 * from where it was copied. If it is the case, it does not copy it.
 */
int paste_file(void) {
    char *copied_file_dir, path[PATH_MAX + 1] = {0};

    for (int i = 0; i < thread_h->num_selected; i++) {
        strncpy(path, thread_h->selected_files[i], PATH_MAX);
        copied_file_dir = dirname(path);
        if (strcmp(thread_h->full_path, copied_file_dir)) {
            cpr(thread_h->selected_files[i]);
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
    char pasted_file[PATH_MAX + 1] = {0}, *copied_file_dir, path[PATH_MAX + 1] = {0};
    struct stat file_stat_copied, file_stat_pasted;

    lstat(thread_h->full_path, &file_stat_pasted);
    for (int i = 0; i < thread_h->num_selected; i++) {
        strncpy(path, thread_h->selected_files[i], PATH_MAX);
        copied_file_dir = dirname(path);
        if (strcmp(thread_h->full_path, copied_file_dir)) {
            lstat(copied_file_dir, &file_stat_copied);
            if (file_stat_copied.st_dev == file_stat_pasted.st_dev) { // if on the same fs, just rename the file
                snprintf(pasted_file, PATH_MAX, "%s%s", 
                         thread_h->full_path, 
                         strrchr(thread_h->selected_files[i], '/'));
                if (rename(thread_h->selected_files[i], pasted_file) == - 1) {
                    print_info(strerror(errno), ERR_LINE);
                }
            } else { // copy file and remove original file
                cpr(thread_h->selected_files[i]);
                rmrf(thread_h->selected_files[i]);
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
static void cpr(const char *tmp) {
    char path[PATH_MAX + 1] = {0};
    
    strncpy(path, tmp, PATH_MAX);
    distance_from_root = strlen(dirname(path));
    nftw(tmp, recursive_copy, 64, FTW_MOUNT | FTW_PHYS);
}

static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int len, fd_to, fd_from, ret = 0;
    char pasted_file[PATH_MAX + 1] = {0};

    snprintf(pasted_file, PATH_MAX, "%s%s", thread_h->full_path, path + distance_from_root);
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
void fast_browse(wint_t c) {
    static struct timeval timer;
    static wchar_t fast_browse_str[NAME_MAX + 1] = {0};
    
    const int FAST_BROWSE_THRESHOLD = 500000; // 0.5s
    const int MILLION = 1000000;
    
    int i = 0;
    char mbstr[NAME_MAX + 1] = {0};
    uint64_t diff = (MILLION * timer.tv_sec) + timer.tv_usec;
   
    gettimeofday(&timer, NULL);
    diff = MILLION * (timer.tv_sec) + timer.tv_usec - diff;
    if ((diff < FAST_BROWSE_THRESHOLD) && (wcslen(fast_browse_str))) {
        i = ps[active].curr_pos;
    } else {
        wmemset(fast_browse_str, 0, NAME_MAX + 1);
    }
    fast_browse_str[wcslen(fast_browse_str)] = c;
    wcstombs(mbstr, fast_browse_str, NAME_MAX + 1);
    print_info(mbstr, INFO_LINE);
    if (!move_cursor_to_file(i, mbstr, active)) {
        wmemset(fast_browse_str, 0, NAME_MAX + 1);
    }
}

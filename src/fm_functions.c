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

static void open_file(const char *str);
static void iso_mount_service(const char *str);
static int remove_from_list(const char *name);
static file_list *select_file(char c, file_list *h);
static void *cpr(void *x);
static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void check_pasted(void);
static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int rmrf(const char *path);
static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int search_inside_archive(const char *path, int i);
static int search_file(const char *path);
static void *search_thread(void *x);
static void *print_file(void *filename);
static void *archiver_func(void *x);
static int recursive_archive(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void try_extractor(const char *path);
static void *extractor_thread(void *a);
static int shasum_func(unsigned char **hash, long size, unsigned char *buffer);
static int md5sum_func(const char *str, unsigned char **hash, long size, unsigned char *buffer);

static char root_pasted_dir[PATH_MAX];
static struct archive *archive = NULL;
static int distance_from_root;

void change_dir(const char *str)
{
    if (chdir(str) != -1) {
        getcwd(ps[active].my_cwd, PATH_MAX);
        generate_list(active);
        print_info(NULL, INFO_LINE);
    } else {
        print_info(strerror(errno), ERR_LINE);
    }
}

void switch_hidden(void)
{
    int i;
    if (config.show_hidden == 0)
        config.show_hidden = 1;
    else
        config.show_hidden = 0;
    for (i = 0; i < cont; i++)
        generate_list(i);
}

void manage_file(const char *str)
{
    if ((file_isCopied(str)) && ((is_thread_running(paste_th)) || (is_thread_running(archiver_th)))) {
        print_info("This file is being pasted/archived. Cannot open.", ERR_LINE);
        return;
    }
    if (get_mimetype(str, "iso"))
        return iso_mount_service(str);
    if (is_archive(str))
         return try_extractor(str);
    if ((config.editor) && (access(config.editor, X_OK) != -1))
        return open_file(str);
    print_info("You have to specify a valid editor in config file.", ERR_LINE);
}

static void open_file(const char *str)
{
    pid_t pid;
    if ((get_mimetype(str, "text/")) || (get_mimetype(str, "x-empty"))) {
        endwin();
        pid = vfork();
        if (pid == 0)
            execl(config.editor, config.editor, str, NULL);
        else
            waitpid(pid, NULL, 0);
        refresh();
    }
}

static void iso_mount_service(const char *str)
{
    if (access("/usr/bin/fuseiso", F_OK) == -1) {
        print_info("You need fuseiso for iso mounting support.", ERR_LINE);
        return;
    }
    if (access(ps[active].my_cwd, W_OK) != 0) {
        print_info("You do not have write permissions here.", ERR_LINE);
        return;
    }
    pid_t pid;
    char mount_point[strlen(str)];
    strcpy(mount_point, str);
    mount_point[strlen(str) - strlen(strrchr(str, '.'))] = '\0';
    pid = vfork();
    if (pid == 0) {
        if (mkdir(mount_point, ACCESSPERMS) == -1)
            execl("/usr/bin/fusermount", "/usr/bin/fusermount", "-u", mount_point, NULL);
        else
            execl("/usr/bin/fuseiso", "/usr/bin/fuseiso", str, mount_point, NULL);
    } else {
        waitpid(pid, NULL, 0);
        if (rmdir(mount_point) == 0)
            print_info("Succesfully unmounted.", INFO_LINE);
        else
            print_info("Succesfully mounted.", INFO_LINE);
        sync_changes();
    }
}

void new_file(void)
{
    if ((file_isCopied(NULL)) && ((is_thread_running(paste_th)) || (is_thread_running(archiver_th)))) {
        print_info("Your current dir is being pasted/archived. Cannot create new file here.", ERR_LINE);
        return;
    }
    FILE *f;
    const char *mesg = "Insert new file name:> ";
    char str[PATH_MAX];
    ask_user(mesg, str, PATH_MAX, 0);
    if ((strlen(str)) && (access(str, F_OK) == -1)) {
        f = fopen(str, "w");
        fclose(f);
        sync_changes();
        print_info("File created.", INFO_LINE);
    } else {
        print_info("Could not create the file.", ERR_LINE);
    }
}

void remove_file(void)
{
    if (file_isCopied(ps[active].nl[ps[active].curr_pos]))
        return;
    const char *mesg = "Are you serious? y/N:> ";
    char c;
    ask_user(mesg, &c, 1, 'n');
    if (c == 'y') {
        if (rmrf(ps[active].nl[ps[active].curr_pos]) == -1)
            print_info("Could not remove. Check user permissions.", ERR_LINE);
        else {
            sync_changes();
            print_info("File/dir removed.", INFO_LINE);
        }
    }
}

void manage_c_press(char c)
{
    if ((is_thread_running(paste_th)) || (is_thread_running(archiver_th))) {
        print_info("A thread is still running. Wait for it.", INFO_LINE);
        return;
    }
    char str[PATH_MAX];
    sprintf(str, "%s/%s", ps[active].my_cwd, ps[active].nl[ps[active].curr_pos]);
    if ((!selected_files) || (remove_from_list(str) == 0)) {
        selected_files = select_file(c, selected_files);
        print_info("File added to copy list.", INFO_LINE);
    } else {
        if (selected_files)
            print_info("File deleted from copy list.", INFO_LINE);
        else
            print_info("File deleted from copy list. Copy list empty.", INFO_LINE);
    }
}

static int remove_from_list(const char *name)
{
    file_list *tmp = selected_files, *temp = NULL;
    if (strcmp(name, tmp->name) == 0) {
        selected_files = selected_files->next;
        free(tmp);
        return 1;
    }
    while(tmp->next) {
        if (strcmp(name, tmp->next->name) == 0) {
            temp = tmp->next;
            tmp->next = tmp->next->next;
            free(temp);
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

static file_list *select_file(char c, file_list *h)
{
    if (h) {
        h->next = select_file(c, h->next);
    } else {
        if (!(h = safe_malloc(sizeof(struct list), "Memory allocation failed.")))
            return NULL;
        sprintf(h->name, "%s/%s", ps[active].my_cwd, ps[active].nl[ps[active].curr_pos]);
        if (c == 'x')
            h->cut = 1;
        else
            h->cut = 0;
        h->next = NULL;
    }
    return h;
}

void paste_file(void)
{
    const char *thread_running = "There's already a thread working on this file list. Please wait.";
    if ((is_thread_running(paste_th)) || (is_thread_running(archiver_th))) {
        print_info(thread_running, INFO_LINE);
        return;
    }
    if (access(ps[active].my_cwd, W_OK) != 0) {
        print_info("Cannot paste here, check user permissions. Paste somewhere else please.", ERR_LINE);
        return;
    }
    char pasted_file[PATH_MAX], copied_file_dir[PATH_MAX];
    int i = 0;
    struct stat file_stat_copied, file_stat_pasted;
    file_list *tmp = NULL;
    strcpy(root_pasted_dir, ps[active].my_cwd);
    lstat(root_pasted_dir, &file_stat_pasted);
    for(tmp = selected_files; tmp; tmp = tmp->next) {
        strcpy(copied_file_dir, tmp->name);
        copied_file_dir[strlen(tmp->name) - strlen(strrchr(tmp->name, '/'))] = '\0';
        if (strcmp(root_pasted_dir, copied_file_dir) == 0) {
            tmp->cut = CANNOT_PASTE_SAME_DIR;
        } else {
            lstat(copied_file_dir, &file_stat_copied);
            if ((tmp->cut == 1) && (file_stat_copied.st_dev == file_stat_pasted.st_dev)) {
                sprintf(pasted_file, "%s%s", root_pasted_dir, strrchr(tmp->name, '/'));
                tmp->cut = MOVED_FILE;
                if (rename(tmp->name, pasted_file) == - 1)
                    print_info(strerror(errno), ERR_LINE);
            } else {
                i++;
            }
        }
    }
    pthread_create(&paste_th, NULL, cpr, &i);
}

static void *cpr(void *n)
{
    file_list *tmp = selected_files;
    if (*((int *)n) > 0) {
        print_info(NULL, INFO_LINE);
        while (tmp) {
            if ((tmp->cut != MOVED_FILE) && (tmp->cut != CANNOT_PASTE_SAME_DIR)) {
                distance_from_root = strlen(tmp->name) - strlen(strrchr(tmp->name, '/'));
                nftw(tmp->name, recursive_copy, 64, FTW_MOUNT | FTW_PHYS);
            }
            tmp = tmp->next;
        }
    }
    check_pasted();
    return NULL;
}

static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int buff[BUFF_SIZE];
    int len, fd_to, fd_from;
    char pasted_file[PATH_MAX];
    sprintf(pasted_file, "%s%s", root_pasted_dir, path + distance_from_root);
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

static void check_pasted(void)
{
    int i, printed[cont];
    file_list *tmp = selected_files;
    char copied_file_dir[PATH_MAX];
    for (i = 0; i < cont; i++) {
        if (strcmp(root_pasted_dir, ps[i].my_cwd) == 0) {
            generate_list(i);
            printed[i] = 1;
        } else {
            printed[i] = 0;
        }
    }
    while (tmp) {
        if (tmp->cut == 1) {
            if (rmrf(tmp->name) == -1)
                print_info("Could not cut. Check user permissions.", ERR_LINE);
        }
        if ((tmp->cut == 1) || (tmp->cut == MOVED_FILE)) {
            strcpy(copied_file_dir, tmp->name);
            copied_file_dir[strlen(tmp->name) - strlen(strrchr(tmp->name, '/'))] = '\0';
            for (i = 0; i < cont; i++) {
                if ((printed[i] == 0) && (strcmp(copied_file_dir, ps[i].my_cwd) == 0))
                    generate_list(i);
            }
        }
        tmp = tmp->next;
    }
    free_copied_list(selected_files);
    selected_files = NULL;
    print_info("Every files has been copied/moved.", INFO_LINE);
}

void rename_file_folders(void)
{
    if (file_isCopied(ps[active].nl[ps[active].curr_pos]))
        return;
    const char *mesg = "Insert new name:> ";
    char str[PATH_MAX];
    ask_user(mesg, str, PATH_MAX, 0);
    if (!(strlen(str)) || (rename(ps[active].nl[ps[active].curr_pos], str) == - 1)) {
        print_info(strerror(errno), ERR_LINE);
    } else {
        sync_changes();
        print_info("File renamed.", INFO_LINE);
    }
}

void create_dir(void)
{
    if ((file_isCopied(NULL)) && ((is_thread_running(paste_th)) || (is_thread_running(archiver_th)))) {
        print_info("Your current dir is being pasted/archived. Cannot create new dir here.", ERR_LINE);
        return;
    }
    const char *mesg = "Insert new folder name:> ";
    char str[PATH_MAX];
    ask_user(mesg, str, PATH_MAX, 0);
    if (!(strlen(str)) || (mkdir(str, 0700) == - 1)) {
        print_info(strerror(errno), ERR_LINE);
    } else {
        sync_changes();
        print_info("Folder created.", INFO_LINE);
    }
}

static int recursive_remove(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    return remove(path);
}

static int rmrf(const char *path)
{
    if (access(path, W_OK) == 0)
        return nftw(path, recursive_remove, 64, FTW_DEPTH | FTW_PHYS | FTW_MOUNT);
    return -1;
}

static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    const char *memfail = "Stopping search as no more memory can be allocated.";
    char fixed_str[PATH_MAX];
    int i = 0;
    if (ftwbuf->level == 0)
        return 0;
    while ((sv.found_searched[i]) && (i < MAX_NUMBER_OF_FOUND)) {
        i++;
    }
    if (i == MAX_NUMBER_OF_FOUND) {
        free_str(sv.found_searched);
        return 1;
    }
    if ((sv.search_archive) && (is_archive(strrchr(path, '/')))) {
        return search_inside_archive(path, i);
    } else {
        strcpy(fixed_str, strrchr(path, '/') + 1);
        if (strncmp(fixed_str, sv.searched_string, strlen(sv.searched_string)) == 0) {
            if (!(sv.found_searched[i] = safe_malloc(sizeof(char) * PATH_MAX, memfail)))
                return 1;
            strcpy(sv.found_searched[i], path);
            if (typeflag == FTW_D)
                strcat(sv.found_searched[i], "/");
        }
    }
    return 0;
}

static int search_inside_archive(const char *path, int i)
{
    const char *memfail = "Stopping search as no more memory can be allocated.";
    char str[PATH_MAX];
    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    if (archive_read_open_filename(a, path, BUFF_SIZE) == ARCHIVE_OK) {
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            strcpy(str, archive_entry_pathname(entry));
            if (str[strlen(str) - 1] == '/') // check if we're on a dir
                str[strlen(str) - 1] = '\0';
            if (strrchr(str, '/'))
                strcpy(str, strrchr(str, '/') + 1);
            if (strncmp(str, sv.searched_string, strlen(sv.searched_string)) == 0) {
                if (!(sv.found_searched[i] = safe_malloc(sizeof(char) * PATH_MAX, memfail))) {
                    archive_read_free(a);
                    return 1;
                }
                sprintf(sv.found_searched[i], "%s/%s", path, archive_entry_pathname(entry));
                i++;
            }
        }
    }
    archive_read_free(a);
    return 0;
}

static int search_file(const char *path)
{
    return nftw(path, recursive_search, 64, FTW_MOUNT | FTW_PHYS);
}

void search(void)
{
    pthread_t search_th;
    const char *mesg = "Insert filename to be found, at least 5 chars:> ";
    const char *s = "Do you want to search in archives too? Search can result slower and has higher memory usage. y/N:> ";
    char c;
    ask_user(mesg, sv.searched_string, 20, 0);
    if (strlen(sv.searched_string) < 5) {
        print_info("At least 5 chars...", INFO_LINE);
        return;
    }
    ask_user(s, &c, 1, 'n');
    if (c == 'y')
        sv.search_archive = 1;
    sv.searching = 1;
    sv.search_active_win = active;
    print_info(NULL, INFO_LINE);
    pthread_create(&search_th, NULL, search_thread, NULL);
    pthread_detach(search_th);
}

static void *search_thread(void *x)
{
    int ret = search_file(ps[active].my_cwd);
    if (!sv.found_searched[0]) {
        sv.searching = 0;
        sv.search_archive = 0;
        if (ret == 1)
            print_info("Too many files found; try with a larger string.", INFO_LINE);
        else
            print_info("No files found.", INFO_LINE);
    } else {
        sv.searching = 2;
        print_info(NULL, INFO_LINE);
    }
    return NULL;
}

void list_found(void)
{
    int i = 0;
    char str[20];
    sv.searching = 3;
    while (sv.found_searched[i])
        i++;
    ps[active].delta = 0;
    ps[active].curr_pos = 0;
    wclear(ps[active].fm);
    list_everything(active, 0, 0, sv.found_searched);
    sprintf(str, "%d files found.", i);
    print_info(str, INFO_LINE);
    search_loop();
}

void search_loop(void)
{
    char arch_str[PATH_MAX];
    char input;
    const char *mesg = "Open file? Y to open, n to switch to the folder:> ";
    const char *arch_mesg = "This file is inside an archive. Switch to its directory? Y/n:> ";
    int c, len;
    arch_str[0] = '\0';
    do {
        c = wgetch(ps[active].fm);
        switch (c) {
        case KEY_UP:
            scroll_up(sv.found_searched);
            break;
        case KEY_DOWN:
            scroll_down(sv.found_searched);
            break;
        case 10:
            if (sv.search_archive) {
                strcpy(arch_str, sv.found_searched[ps[active].curr_pos]);
                while ((strlen(arch_str)) && (!is_archive(strrchr(arch_str, '/'))))
                    arch_str[strlen(arch_str) - strlen(strrchr(arch_str, '/'))] = '\0';
            }
            len = strlen(strrchr(sv.found_searched[ps[active].curr_pos], '/'));
            if ((!strlen(arch_str)) && (len != 1)) { // is a file
                ask_user(mesg, &input, 1, 'y');
                if (input != 'n') {
                    if (input == 'y') // is a file and user wants to open it
                        manage_file(sv.found_searched[ps[active].curr_pos]);
                    break;
                }
            } // is a dir or an archive or a file but user wants to switch to its dir
            len = strlen(sv.found_searched[ps[active].curr_pos]) - len;
            if (strlen(arch_str)) { // is an archive
                ask_user(arch_mesg, &input, 1, 'y');
                if (input == 'y')
                    len = strlen(arch_str) - strlen(strrchr(arch_str, '/'));
                else
                    break;
            }
            sv.found_searched[ps[active].curr_pos][len] = '\0';
            c = 'q';
            break;
        case 9: // tab to change tab
            if (cont == MAX_TABS) {
                active = 1 - active;
                chdir(ps[active].my_cwd);
                return;
            }
            break;
        case 'q':
            strcpy(sv.found_searched[ps[active].curr_pos], ps[active].my_cwd);
            break;
        }
    } while (c != 'q');
    sv.searching = 0;
    sv.search_archive = 0;
    change_dir(sv.found_searched[ps[active].curr_pos]);
    free_str(sv.found_searched);
}

void print_support(char *str)
{
    if (access("/usr/include/cups/cups.h", F_OK ) == -1) {
        print_info("You must have libcups installed.", ERR_LINE);
        return;
    }
    pthread_t print_thread;
    const char *mesg = "Do you really want to print this file? Y/n:> ";
    char c;
    ask_user(mesg, &c, 1, 'y');
    if (c == 'y') {
        pthread_create(&print_thread, NULL, print_file, str);
        pthread_detach(print_thread);
    }
}

static void *print_file(void *filename)
{
    cups_dest_t *dests, *default_dest;
    int num_dests = cupsGetDests(&dests);
    if (num_dests > 0) {
        default_dest = cupsGetDest(NULL, NULL, num_dests, dests);
        cupsPrintFile(default_dest->name, (char *)filename, "ncursesFM job", default_dest->num_options, default_dest->options);
        print_info("Print job done.", INFO_LINE);
    } else {
        print_info("No printers available.", ERR_LINE);
    }
    return NULL;
}

void create_archive(void)
{
    const char *thread_running = "There's already a thread working on this file list. Please wait.";
    if ((is_thread_running(paste_th)) || (is_thread_running(archiver_th))) {
        print_info(thread_running, INFO_LINE);
        return;
    }
    const char *mesg = "Insert new file name (defaults to first entry name):> ";
    const char *question = "Do you really want to compress these files? Y/n:> ";
    char archive_path[PATH_MAX], str[PATH_MAX], c;
    ask_user(question, &c, 1, 'y');
    if (c == 'y') {
        if (access(ps[active].my_cwd, W_OK) != 0) {
            print_info("No write perms here.", ERR_LINE);
            return;
        }
        archive = archive_write_new();
        if ((archive_write_add_filter_gzip(archive) == ARCHIVE_FATAL) || (archive_write_set_format_pax_restricted(archive) == ARCHIVE_FATAL)) {
            print_info(archive_error_string(archive), ERR_LINE);
            archive_write_free(archive);
            archive = NULL;
            return;
        }
        ask_user(mesg, str, PATH_MAX, 0);
        if (!strlen(str))
            strcpy(str, strrchr(selected_files->name, '/') + 1);
        sprintf(archive_path, "%s/%s.tgz", ps[active].my_cwd, str);
        if (archive_write_open_filename(archive, archive_path) == ARCHIVE_FATAL) {
            print_info(strerror(archive_errno(archive)), ERR_LINE);
            archive_write_free(archive);
            archive = NULL;
            return;
        }
        pthread_create(&archiver_th, NULL, archiver_func, (void *)archive_path);
    }
}

static void *archiver_func(void *archive_path)
{
    file_list *tmp = selected_files;
    char str[PATH_MAX];
    int i;
    print_info(NULL, INFO_LINE);
    while (tmp) {
        distance_from_root = strlen(tmp->name) - strlen(strrchr(tmp->name, '/'));
        nftw(tmp->name, recursive_archive, 64, FTW_MOUNT | FTW_PHYS);
        tmp = tmp->next;
    }
    archive_write_free(archive);
    archive = NULL;
    strcpy(str, archive_path);
    str[strlen(archive_path) - strlen(strrchr(archive_path, '/'))] = '\0';
    for (i = 0; i < cont; i++) {
        if (strcmp(str, ps[i].my_cwd) == 0)
            generate_list(i);
    }
    print_info("The archive is ready.", INFO_LINE);
    free_copied_list(selected_files);
    selected_files = NULL;
    return NULL;
}

static int recursive_archive(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    char buff[BUFF_SIZE], entry_name[PATH_MAX];
    int len, fd;
    struct archive_entry *entry = archive_entry_new();
    strcpy(entry_name, path + distance_from_root + 1);
    archive_entry_set_pathname(entry, entry_name);
    archive_entry_copy_stat(entry, sb);
    archive_write_header(archive, entry);
    archive_entry_free(entry);
    fd = open(path, O_RDONLY);
    if (fd != -1) {
        len = read(fd, buff, sizeof(buff));
        while (len > 0) {
            archive_write_data(archive, buff, len);
            len = read(fd, buff, sizeof(buff));
        }
        close(fd);
    }
    return 0;
}

static void try_extractor(const char *path)
{
    const char *extr_thread_running = "There's already an extractig operation. Currently only one is supported. Please wait.";
    if (is_thread_running(extractor_th)) {
        print_info(extr_thread_running, INFO_LINE);
        return;
    }
    struct archive *a;
    const char *question = "Do you really want to extract this archive? Y/n:> ";
    char c;
    ask_user(question, &c, 1, 'y');
    if (c == 'y') {
        if (access(ps[active].my_cwd, W_OK) != 0) {
            print_info("No write perms here.", ERR_LINE);
            return;
        }
        a = archive_read_new();
        archive_read_support_filter_all(a);
        archive_read_support_format_all(a);
        if (archive_read_open_filename(a, path, BUFF_SIZE) == ARCHIVE_OK) {
            pthread_create(&extractor_th, NULL, extractor_thread, a);
        } else {
            print_info(archive_error_string(a), ERR_LINE);
            archive_read_free(a);
        }
    }
}

static void *extractor_thread(void *a)
{
    struct archive *ext;
    struct archive_entry *entry;
    int flags, len, i;
    char buff[BUFF_SIZE], current_dir[PATH_MAX];
    extracting = 1;
    print_info(NULL, INFO_LINE);
    strcpy(current_dir, ps[active].my_cwd);
    ext = archive_write_disk_new();
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    while (archive_read_next_header(a, &entry) != ARCHIVE_EOF) {
        archive_write_header(ext, entry);
        len = archive_read_data(a, buff, sizeof(buff));
        while (len > 0) {
            archive_write_data(ext, buff, len);
            len = archive_read_data(a, buff, sizeof(buff));
        }
    }
    archive_read_free(a);
    archive_write_free(ext);
    for (i = 0; i < cont; i++) {
        if (strcmp(ps[i].my_cwd, current_dir) == 0)
            generate_list(i);
    }
    extracting = 0;
    print_info("Succesfully extracted.", INFO_LINE);
    return NULL;
}

void integrity_check(const char *str)
{
    const char *question = "Shasum (1, default) or md5sum (2)?> ";
    unsigned char *hash = NULL, *buffer = NULL;
    int length, i;
    FILE *fp;
    long size;
    char c;
    ask_user(question, &c, 1, '1');
    if ((c == '1') || (c == '2')) {
        if ((c == '1') && (access("/usr/include/openssl/sha.h", F_OK ) == -1)) {
            print_info("You must have openssl installed.", ERR_LINE);
            return;
        }
        if ((c == '2') && (access("/usr/include/openssl/md5.h", F_OK ) == -1)) {
            print_info("You must have openssl installed.", ERR_LINE);
            return;
        }
        if(!(fp = fopen(str, "rb"))) {
            print_info(strerror(errno), ERR_LINE);
            return;
        }
        fseek(fp, 0L, SEEK_END);
        size = ftell(fp);
        rewind(fp);
        if (!(buffer = safe_malloc(size, "Memory allocation failed."))) {
            fclose(fp);
            return;
        }
        fread(buffer, size, 1, fp);
        fclose(fp);
        if (c == '1')
            length = shasum_func(&hash, size, buffer);
        else if (c == '2')
            length = md5sum_func(str, &hash, size, buffer);
        free(buffer);
        char s[2 * length];
        s[0] = '\0';
        for(i = 0; i < length; i++)
            sprintf(s + strlen(s), "%02x", hash[i]);
        print_info(s, INFO_LINE);
        free(hash);
    }
}

static int shasum_func(unsigned char **hash, long size, unsigned char *buffer)
{
    int i = 1, length = SHA_DIGEST_LENGTH;
    char input[4];
    const char *question = "Which shasum do you want? Choose between 1, 224, 256, 384, 512. Defaults to 1.> ";
    ask_user(question, input, 4, 0);
    if (strlen(input))
        i = atoi(input);
    if ((i == 224) || (i == 256) || (i == 384) || (i == 512))
        length = i / 8;
    if (!(*hash = safe_malloc(sizeof(unsigned char) * length, "Memory allocation failed.")))
        return 0;
    switch(i) {
    case 224:
        SHA224(buffer, size, *hash);
        break;
    case 256:
        SHA256(buffer, size, *hash);
        break;
    case 384:
        SHA384(buffer, size, *hash);
        break;
    case 512:
        SHA512(buffer, size, *hash);
        break;
    default:
        SHA1(buffer, size, *hash);
        break;
    }
    return length;
}

static int md5sum_func(const char *str, unsigned char **hash, long size, unsigned char *buffer)
{
    struct stat file_stat;
    const char *question = "This file is quite large. Md5 sum can take very long time (up to some minutes). Continue? Y/n";
    char c = 'y';
    int ret = 0;
    lstat(str, &file_stat);
    if (file_stat.st_size > 100 * 1024 * 1024) // 100 MB
        ask_user(question, &c, 1, 'y');
    if (c == 'y') {
        if (!(*hash = safe_malloc(sizeof(unsigned char) * MD5_DIGEST_LENGTH, "Memory allocation failed.")))
            return ret;
        MD5(buffer, size, *hash);
        ret = MD5_DIGEST_LENGTH;
    }
    return ret;
}
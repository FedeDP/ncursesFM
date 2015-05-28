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

static const char *iso_extensions[] = {".iso", ".bin", ".nrg", ".img", ".mdf"};
static const char *archive_extensions[] = {".tgz", ".tar.gz", ".zip", ".rar", ".xz", ".ar"}; // add other supported extensions
static char root_dir[PATH_MAX];
static struct archive *archive = NULL;
static int search_archive = 0;

void change_dir(char *str)
{
    chdir(str);
    getcwd(ps[active].my_cwd, PATH_MAX);
    generate_list(active);
    print_info(NULL, INFO_LINE);
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

void manage_file(char *str)
{
    int dim;
    if ((searching != 3) && (file_isCopied(str)))
        return;
    dim = is_extension(str, iso_extensions);
    if (dim) {
        iso_mount_service(str, dim);
    } else {
        if (is_extension(str, archive_extensions)) {
            try_extractor(str);
        } else {
            if ((config.editor) && (access(config.editor, X_OK) != -1))
                open_file(str);
            else
                print_info("You have to specify a valid editor in config file.", ERR_LINE);
        }
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

static void iso_mount_service(char *str, int dim)
{
    pid_t pid;
    char mount_point[strlen(str) - dim + 1];
    if (access("/usr/bin/fuseiso", F_OK) == -1) {
        print_info("You need fuseiso for iso mounting support.", ERR_LINE);
        return;
    }
    if (access(ps[active].my_cwd, W_OK) != 0) {
        print_info("You do not have write permissions here.", ERR_LINE);
        return;
    }
    strncpy(mount_point, str, strlen(str) - dim);
    mount_point[strlen(str) - dim] = '\0';
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
    FILE *f;
    const char *mesg = "Insert new file name:> ";
    char str[PATH_MAX];
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, str);
    if (access(str, F_OK) == -1) {
        f = fopen(str, "w");
        fclose(f);
        sync_changes();
        print_info("File created.", INFO_LINE);
    } else {
        print_info("A file with this name already exists in this folder.", ERR_LINE);
    }
    noecho();
}

void remove_file(void)
{
    const char *mesg = "Are you serious? y/n:> ";
    if (file_isCopied(ps[active].nl[ps[active].curr_pos]))
        return;
    if (ask_user(mesg) == 1) {
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
    char str[PATH_MAX];
    if ((th) && (pthread_kill(th, 0) != ESRCH)) {
        print_info("A thread is still running. Wait for it.", INFO_LINE);
        return;
    }
    strcpy(str, ps[active].my_cwd);
    strcat(str, "/");
    strcat(str, ps[active].nl[ps[active].curr_pos]);
    if ((!selected_files) || (remove_from_list(str) == 0)) {
        strcpy(info_message, "There are selected files.");
        selected_files = select_file(c, selected_files);
    } else {
        if (selected_files)
            print_info("File deleted from copy list.", INFO_LINE);
        else {
            memset(info_message, 0, strlen(info_message));
            print_info("File deleted from copy list. Copy list empty.", INFO_LINE);
        }
    }
}

static int remove_from_list(char *name)
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
        strcpy(h->name, ps[active].my_cwd);
        strcat(h->name, "/");
        strcat(h->name, ps[active].nl[ps[active].curr_pos]);
        h->cut = 0;
        if (c == 'x')
            h->cut = 1;
        h->next = NULL;
        print_info("File added to copy list.", INFO_LINE);
    }
    return h;
}

void paste_file(void)
{
    char pasted_file[PATH_MAX], copied_file_dir[PATH_MAX];
    int cont = 0;
    struct stat file_stat_copied, file_stat_pasted;
    file_list *tmp = NULL, *temp = NULL;
    strcpy(root_dir, ps[active].my_cwd);
    if (access(root_dir, W_OK) != 0) {
        print_info("Cannot paste here, check user permissions. Paste somewhere else please.", ERR_LINE);
        return;
    }
    memset(info_message, 0, strlen(info_message));
    stat(root_dir, &file_stat_pasted);
    for(tmp = selected_files; tmp; tmp = tmp->next) {
        strcpy(copied_file_dir, tmp->name);
        copied_file_dir[strlen(tmp->name) - strlen(strrchr(tmp->name, '/'))] = '\0';
        if (strcmp(root_dir, copied_file_dir) == 0) {
            tmp->cut = CANNOT_PASTE_SAME_DIR;
        } else {
            stat(copied_file_dir, &file_stat_copied);
            if ((tmp->cut == 1) && (file_stat_copied.st_dev == file_stat_pasted.st_dev)) {
                strcpy(pasted_file, root_dir);
                strcat(pasted_file, strrchr(tmp->name, '/'));
                tmp->cut = MOVED_FILE;
                if (rename(tmp->name, pasted_file) == - 1)
                    print_info(strerror(errno), ERR_LINE);
            } else {
                cont++;
            }
        }
    }
    if (cont > 0)
        pthread_create(&th, NULL, cpr, &cont);
    else
        check_pasted();
}

static void *cpr(void *n)
{
    file_list *tmp = selected_files;
    char old_root_dir[PATH_MAX];
    int i = 0;
    strcpy(old_root_dir, root_dir);
    while (tmp) {
        if ((tmp->cut != MOVED_FILE) && (tmp->cut != CANNOT_PASTE_SAME_DIR)) {
            sprintf(info_message, "Pasting file %d of %d", ++i, *((int *)n));
            print_info(NULL, INFO_LINE);
            strcat(root_dir, strrchr(tmp->name, '/'));
            nftw(tmp->name, recursive_copy, 64, FTW_MOUNT | FTW_PHYS);
            strcpy(root_dir, old_root_dir);
        }
        tmp = tmp->next;
    }
    memset(info_message, 0, strlen(info_message));
    check_pasted();
}

static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int buff[BUFF_SIZE];
    int len, fd_to, fd_from;
    char pasted_file[PATH_MAX];
    strcpy(pasted_file, root_dir);
    pasted_file[strlen(pasted_file) - strlen(strrchr(pasted_file, '/'))] = '\0';
    strcat(pasted_file, strrstr(path, strrchr(root_dir, '/')));
    if (typeflag == FTW_D) {
        mkdir(pasted_file, sb->st_mode);
    } else {
        fd_to = open(pasted_file, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC, sb->st_mode);
        fd_from = open(path, O_RDONLY);
        len = read(fd_from, buff, sizeof(buff));
        while (len > 0) {
            write(fd_to, buff, len);
            len = read(fd_from, buff, sizeof(buff));
        }
        close(fd_to);
        close(fd_from);
    }
}

static void check_pasted(void)
{
    int i, printed[cont];
    file_list *tmp = selected_files;
    char copied_file_dir[PATH_MAX];
    if (searching != 3) {
        for (i = 0; i < cont; i++) {
            if (strcmp(root_dir, ps[i].my_cwd) == 0) {
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
    }
    print_info("Every files has been copied/moved.", INFO_LINE);
    free_copied_list(selected_files);
    selected_files = NULL;
}

void rename_file_folders(void)
{
    const char *mesg = "Insert new name:> ";
    char str[PATH_MAX];
    if (file_isCopied(ps[active].nl[ps[active].curr_pos]))
        return;
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, str);
    noecho();
    if (rename(ps[active].nl[ps[active].curr_pos], str) == - 1) {
        print_info(strerror(errno), ERR_LINE);
    } else {
        sync_changes();
        print_info("File renamed.", INFO_LINE);
    }
}

void create_dir(void)
{
    const char *mesg = "Insert new folder name:> ";
    char str[PATH_MAX];
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, str);
    noecho();
    if (mkdir(str, 0700) == - 1) {
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

static int rmrf(char *path)
{
    if (access(path, W_OK) == 0)
        return nftw(path, recursive_remove, 64, FTW_DEPTH | FTW_PHYS | FTW_MOUNT);
    return -1;
}

static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    char fixed_str[PATH_MAX];
    int i = 0;
    if (ftwbuf->level == 0)
        return 0;
    while (found_searched[i]) {
        i++;
    }
    if (i >= MAX_NUMBER_OF_FOUND) {
        free_found();
        return 1;
    }
    if ((search_archive) && (is_extension(path, archive_extensions))) {
        search_inside_archive(path, i);
    } else {
        strcpy(fixed_str, strrchr(path, '/'));
        memmove(fixed_str, fixed_str + 1, strlen(fixed_str));
        if (strncmp(fixed_str, searched_string, strlen(searched_string)) == 0) {
            if (!(found_searched[i] = safe_malloc(sizeof(char) * PATH_MAX, "Stopping search as no more memory can be allocated")))
                return 1;
            strcpy(found_searched[i], path);
            if (typeflag == FTW_D)
                strcat(found_searched[i], "/");
        }
    }
    return 0;
}

static void search_inside_archive(const char *path, int i)
{
    char str[PATH_MAX], *str_ptr = NULL;
    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    if (archive_read_open_filename(a, path, BUFF_SIZE) == ARCHIVE_OK) {
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            strcpy(str, archive_entry_pathname(entry));
            str_ptr = strrchr(str, '/');
            if ((str_ptr) && (strlen(str_ptr) == 1)) // check if we're on a dir
                str[strlen(str) - 1] = '\0';
            if (strrchr(str, '/')) {
                strcpy(str, strrchr(str, '/'));
                memmove(str, str + 1, strlen(str));
            }
            if (strncmp(str, searched_string, strlen(searched_string)) == 0) {
                if (!(found_searched[i] = safe_malloc(sizeof(char) * PATH_MAX, "Memory allocation failed.")))
                    return;
                strcpy(found_searched[i], path);
                strcat(found_searched[i], "/");
                strcat(found_searched[i], archive_entry_pathname(entry));
                i++;
            }
        }
    }
    archive_read_free(a);
}

static int search_file(char *path)
{
    return nftw(path, recursive_search, 64, FTW_MOUNT | FTW_PHYS);
}

void search(void)
{
    pthread_t search_th;
    const char *mesg = "Insert filename to be found, at least 5 chars:> ";
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, searched_string);
    noecho();
    if (strlen(searched_string) < 5) {
        print_info("At least 5 chars...", INFO_LINE);
        return;
    }
    if (ask_user("Do you want to search in archives too? Search can result slower and has higher memory usage. y/n") == 1)
        search_archive = 1;
    searching = 1;
    print_info(NULL, INFO_LINE);
    pthread_create(&search_th, NULL, search_thread, NULL);
    pthread_detach(search_th);
}

static void *search_thread(void *x)
{
    int ret = search_file(ps[active].my_cwd);
    if (!found_searched[0]) {
        searching = 0;
        if (ret == 1)
            print_info("Too many files found; try with a larger string.", INFO_LINE);
        else
            print_info("No files found.", INFO_LINE);
        return NULL;
    }
    searching = 2;
    print_info(NULL, INFO_LINE);
}

void list_found(void)
{
    int i = 0, old_size = ps[active].number_of_files;
    char str[20];
    searching = 3;
    while (found_searched[i])
        i++;
    ps[active].number_of_files = i;
    ps[active].delta = 0;
    ps[active].curr_pos = 0;
    wclear(ps[active].fm);
    list_everything(active, 0, dim - 2, found_searched);
    sprintf(str, "%d files found.", i);
    print_info(str, INFO_LINE);
    search_loop();
    searching = 0;
    ps[active].number_of_files = old_size;
    change_dir(found_searched[ps[active].curr_pos]);
    free_found();
}

static void search_loop(void)
{
    char arch_str[PATH_MAX];
    const char *mesg = "Open file? y to open, n to switch to the folder";
    const char *arch_mesg = "This file is inside an archive; do you want to switch to its directory? y/n.";
    int c, len;
    do {
        c = wgetch(ps[active].fm);
        switch (c) {
        case KEY_UP:
            scroll_up(found_searched);
            break;
        case KEY_DOWN:
            scroll_down(found_searched);
            break;
        case 10:
            strcpy(arch_str, found_searched[ps[active].curr_pos]);
            while ((strlen(arch_str)) && (!is_extension(arch_str, archive_extensions)))
                arch_str[strlen(arch_str) - strlen(strrchr(arch_str, '/'))] = '\0';
            len = strlen(strrchr(found_searched[ps[active].curr_pos], '/'));
            if ((!strlen(arch_str)) && (len != 1) && (ask_user(mesg) == 1)) {  // is a file and user wants to open it
                manage_file(found_searched[ps[active].curr_pos]);
            } else {    // is a dir or an archive or a file but user wanted to switch to its dir
                len = strlen(found_searched[ps[active].curr_pos]) - len;
                if (strlen(arch_str)) {
                    if (ask_user(arch_mesg) == 1) // is archive
                        len = strlen(arch_str) - strlen(strrchr(arch_str, '/'));
                    else
                        break;
                }
                found_searched[ps[active].curr_pos][len] = '\0';
                c = 'q';
            }
            break;
        case 'q':
            strcpy(found_searched[ps[active].curr_pos], ps[active].my_cwd);
            break;
        }
    } while (c != 'q');
}

void print_support(char *str)
{
    pthread_t print_thread;
    const char *mesg = "Do you really want to print this file? y/n:> ";
    if (access("/usr/include/cups/cups.h", F_OK ) == -1) {
        print_info("You must have libcups installed.", ERR_LINE);
        return;
    }
    if (ask_user(mesg) == 1) {
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
}

void create_archive(void)
{
    const char *mesg = "Insert new file name:> ";
    char archive_path[PATH_MAX], str[PATH_MAX];
    if (ask_user("Do you really want to compress these files?") == 1) {
        if (access("/usr/include/archive.h", F_OK) == -1) {
            print_info("You must have libarchive installed.", ERR_LINE);
            return;
        }
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
        echo();
        print_info(mesg, INFO_LINE);
        wgetstr(info_win, str);
        strcpy(archive_path, ps[active].my_cwd);
        strcat(archive_path, "/");
        strcat(archive_path, str);
        strcat(archive_path, ".tgz");
        noecho();
        if (archive_write_open_filename(archive, archive_path) == ARCHIVE_FATAL) {
            print_info(strerror(archive_errno(archive)), ERR_LINE);
            archive_write_free(archive);
            archive = NULL;
            return;
        }
        pthread_create(&th, NULL, archiver_func, (void *)archive_path);
    }
}

static void *archiver_func(void *archive_path)
{
    file_list *tmp = selected_files;
    char str[PATH_MAX];
    int i;
    while (tmp) {
        strcpy(info_message, "Archiving...");
        print_info(NULL, INFO_LINE);
        strcpy(root_dir, strrchr(tmp->name, '/'));
        memmove(root_dir, root_dir + 1, strlen(root_dir));
        nftw(tmp->name, recursive_archive, 64, FTW_MOUNT | FTW_PHYS);
        tmp = tmp->next;
    }
    archive_write_free(archive);
    archive = NULL;
    if (searching != 3) {
        strcpy(str, archive_path);
        str[strlen(str) - strlen(strrchr(str, '/'))] = '\0';
        for (i = 0; i < cont; i++) {
            if (strcmp(str, ps[i].my_cwd) == 0)
                generate_list(i);
        }
    }
    memset(info_message, 0, strlen(info_message));
    print_info("The archive is ready.", INFO_LINE);
    free_copied_list(selected_files);
    selected_files = NULL;
}

static int recursive_archive(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    char buff[BUFF_SIZE], entry_name[PATH_MAX];
    int len, fd;
    struct archive_entry *entry = archive_entry_new();
    strcpy(entry_name, strrstr(path, root_dir));
    archive_entry_set_pathname(entry, entry_name);
    archive_entry_copy_stat(entry, sb);
    archive_write_header(archive, entry);
    archive_entry_free(entry);
    fd = open(path, O_RDONLY);
    len = read(fd, buff, sizeof(buff));
    while (len > 0) {
        archive_write_data(archive, buff, len);
        len = read(fd, buff, sizeof(buff));
    }
    close(fd);
}

static void try_extractor(char *path)
{
    struct archive *a;
    if (ask_user("Do you really want to extract this archive?") == 1) {
        if (access("/usr/include/archive.h", F_OK) == -1) {
            print_info("You must have libarchive installed.", ERR_LINE);
            return;
        }
        if (access(ps[active].my_cwd, W_OK) != 0) {
            print_info("No write perms here.", ERR_LINE);
            return;
        }
        a = archive_read_new();
        archive_read_support_filter_all(a);
        archive_read_support_format_all(a);
        if (archive_read_open_filename(a, path, BUFF_SIZE) != ARCHIVE_OK) {
            print_info(archive_error_string(a), ERR_LINE);
            archive_read_free(a);
            return;
        }
        extracting = 1;
        print_info(NULL, INFO_LINE);
        pthread_create(&extractor_th, NULL, extractor_thread, a);
    }
}

static void *extractor_thread(void *a)
{
    struct archive *ext;
    struct archive_entry *entry;
    int flags, len, i;
    char buff[BUFF_SIZE], current_dir[PATH_MAX];
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
    if (searching != 3) {
        for (i = 0; i < cont; i++) {
            if (strcmp(ps[i].my_cwd, current_dir) == 0)
                generate_list(i);
        }
    }
    extracting = 0;
    print_info("Succesfully extracted.", INFO_LINE);
}

void shasum_func(const char *str)
{
    int i, length = SHA_DIGEST_LENGTH;
    FILE *fp;
    long size;
    char *buffer, temp[2], input[4];
    if (access("/usr/include/openssl/sha.h", F_OK ) == -1) {
        print_info("You must have openssl installed.", ERR_LINE);
        return;
    }
    const char *question = "Which shasum do you want? Choose between 1, 224, 256, 384, 512. Defaults to 1. > ";
    echo();
    print_info(question, INFO_LINE);
    wgetstr(info_win, input);
    noecho();
    print_info(NULL, INFO_LINE);
    i = atoi(input);
    if ((i == 224) || (i == 256) || (i == 384) || (i == 512))
        length = i / 8;
    unsigned char hash[length];
    char s[2 * length];
    s[0] = '\0';
    if(!(fp = fopen(str, "rb"))) {
        print_info("Could not open this file.", ERR_LINE);
        return;
    }
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    if (buffer = safe_malloc(size, "Memory allocation failed.")) {
        if (fread(buffer, size, 1, fp) != 1) {
            fclose(fp);
            free(buffer);
            print_info("File read failed.", ERR_LINE);
            return;
        }
        switch(i) {
        case 224:
            SHA224(buffer, size, hash);
            break;
        case 256:
            SHA256(buffer, size, hash);
            break;
        case 384:
            SHA384(buffer, size, hash);
            break;
        case 512:
            SHA512(buffer, size, hash);
            break;
        default:
            SHA1(buffer, size, hash);
            break;
        }
        for(i = 0; i < length; i++) {
            sprintf(temp, "%02x", hash[i]);
            strcat(s, temp);
        }
        print_info(s, INFO_LINE);
        free(buffer);
    }
    fclose(fp);
}
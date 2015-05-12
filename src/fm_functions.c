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

static char *found_searched[PATH_MAX];
static char searched_string[PATH_MAX];
static char pasted_dir[PATH_MAX];
static int old_level, search_mode = 0;
static struct archive *archive = NULL;

void change_dir(char *str)
{
    chdir(str);
    getcwd(ps[active].my_cwd, PATH_MAX);
    list_everything(active, 0, dim - 2, 1, 1);
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
        list_everything(i, 0, dim - 2, 1, 1);
}

void manage_file(char *str)
{
    int dim;
    if (file_isCopied())
        return;
    dim = isArchive(str);
    if (dim) {
        mount_service(str, dim);
    } else {
        if ((config.editor) && (access(config.editor, X_OK) != -1))
            open_file(str);
        else
            print_info("You have to specify a valid editor in config file.", ERR_LINE);
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

static void mount_service(char *str, int dim)
{
    pid_t pid;
    char mount_point[strlen(str) - dim + 1];
    if (access("/usr/bin/archivemount", F_OK) != -1) {
        strncpy(mount_point, str, strlen(str) - dim); //check
        mount_point[strlen(str) - dim] = '\0';
        pid = vfork();
        if (pid == 0) {
            if (mkdir(mount_point, ACCESSPERMS) == -1)
                execl("/usr/bin/fusermount", "/usr/bin/fusermount", "-u", mount_point, NULL);
            else
                execl("/usr/bin/archivemount", "/usr/bin/archivemount", str, mount_point, NULL);
        } else {
            waitpid(pid, NULL, 0);
            if (rmdir(mount_point) == 0)
                print_info("Succesfully unmounted.", INFO_LINE);
            else
                print_info("Succesfully mounted.", INFO_LINE);
            sync_changes();
        }
    } else {
        print_info("You need archivemount for mounting support.", ERR_LINE);
    }
}

void new_file(void)
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
        sync_changes();
        print_info("File created.", INFO_LINE);
    }
    noecho();
}

void remove_file(void)
{
    char *mesg = "Are you serious? y/n:> ";
    if (file_isCopied())
        return;
    if (ask_user(mesg) == 1) {
        if (rmrf(ps[active].namelist[ps[active].current_position]->d_name) == -1)
            print_info("Could not remove. Check user permissions.", ERR_LINE);
        else {
            sync_changes();
            print_info("File/dir removed.", INFO_LINE);
        }
    }
}

void manage_c_press(char c)
{
    if ((th) && (pthread_kill(th, 0) != ESRCH)) {
        print_info("A thread is still running. Wait for it.", INFO_LINE);
        return;
    }
    if (remove_from_list(ps[active].namelist[ps[active].current_position]->d_name) == 0) {
        info_message = malloc(strlen("There are selected files."));
        strcpy(info_message, "There are selected files.");
        select_file(c);
    } else {
        if (selected_files)
            print_info("File deleted from copy list.", INFO_LINE);
        else {
            free(info_message);
            info_message = NULL;
            print_info("File deleted from copy list. Copy list empty.", INFO_LINE);
        }
    }
}

static int remove_from_list(char *name)
{
    file_list *tmp = selected_files, *temp = NULL;
    char str[PATH_MAX];
    if (!selected_files)
        return 0;
    strcpy(str, strrchr(selected_files->name, '/'));
    memmove(str, str + 1, strlen(str));
    if (strcmp(str, name) == 0) {
        selected_files = selected_files->next;
        free(tmp);
        return 1;
    }
    memset(str, 0, strlen(str));
    while(tmp->next) {
        strcpy(str, strrchr(tmp->next->name, '/'));
        memmove(str, str + 1, strlen(str));
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

static void select_file(char c)
{
    file_list *tmp;
    if (selected_files) {
        tmp = selected_files;
        while(tmp->next)
            tmp = tmp->next;
        tmp->next = malloc(sizeof(struct list));
        get_full_path(tmp->next->name, ps[active].current_position, active);
        strcpy(tmp->next->dir, ps[active].my_cwd);
        tmp->next->cut = 0;
        if (c == 'x')
            tmp->next->cut = 1;
        tmp->next->next = NULL;
    } else {
        selected_files = malloc(sizeof(struct list));
        get_full_path(selected_files->name, ps[active].current_position, active);
        strcpy(selected_files->dir, ps[active].my_cwd);
        selected_files->cut = 0;
        if (c == 'x')
            selected_files->cut = 1;
        selected_files->next = NULL;
    }
    print_info("File added to copy list.", INFO_LINE);
}

/*
 * Quite hard to understand.
 * Fist I check if we have write permissions in pasted_dir. Otherwise we'll print an error message.
 * Then for each copied file I check if pasted_dir is the same dir in which we're copying. If it's the same, i'll mark it (tmp->cut = -2).
 * Then I check if files are on the same fs and the action is "cut" (and not paste). If that's the case, i'll just "rename" the file
 * and set tmp->cut = -1 (needed in check_pasted).
 * For remaining selected_files, a paste thread will be executed.
 */
void paste_file(void)
{
    char pasted_file[PATH_MAX];
    int size = 0;
    struct stat file_stat_copied, file_stat_pasted;
    file_list *tmp = selected_files, *temp = NULL;
    strcpy(pasted_dir, ps[active].my_cwd);
    if (access(pasted_dir, W_OK) == 0) {
        free(info_message);
        info_message = NULL;
        stat(pasted_dir, &file_stat_pasted);
        for(; tmp; tmp = tmp->next) {
            if (strcmp(pasted_dir, tmp->dir) == 0) {
                tmp->cut = -2;
            } else {
                stat(tmp->dir, &file_stat_copied);
                if ((tmp->cut == 1) && (file_stat_copied.st_dev == file_stat_pasted.st_dev)) {
                    strcpy(pasted_file, pasted_dir);
                    strcat(pasted_file, strrchr(tmp->name, '/'));
                    tmp->cut = -1;
                    if (rename(tmp->name, pasted_file) == - 1)
                        print_info(strerror(errno), ERR_LINE);
                } else {
                    size++;
                }
            }
        }
        if (size > 0)
            pthread_create(&th, NULL, cpr, NULL);
        else
            check_pasted();
    } else {
        print_info("Cannot paste here, check user permissions. Paste somewhere else please.", ERR_LINE);
    }
}

static void *cpr(void *x)
{
    file_list *tmp = selected_files;
    char old_pasted_dir[PATH_MAX];
    int i = 0, n = 0;
    strcpy(old_pasted_dir, pasted_dir);
    for (; tmp; tmp = tmp->next) {
        if ((tmp->cut != -1) && (tmp->cut != -2))
            n++;
    }
    tmp = selected_files;
    while (tmp) {
        if ((tmp->cut != -1) && (tmp->cut != -2)) {
            old_level = 0;
            info_message = malloc(strlen("Pasting file %d of %d"));
            sprintf(info_message, "Pasting file %d of %d", i + 1, n);
            print_info(NULL, INFO_LINE);
            nftw(tmp->name, recursive_copy, 64, FTW_MOUNT | FTW_PHYS);
            strcpy(pasted_dir, old_pasted_dir);
        }
        i++;
        tmp = tmp->next;
    }
    free(info_message);
    info_message = NULL;
    check_pasted();
    return NULL;
}

static int recursive_copy(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int buff[8192];
    int len, fd_to, fd_from;
    char pasted_file[PATH_MAX];
    char old_dir[strlen(path) - strlen(strrchr(path, '/')) + 1];
    if (ftwbuf->level != old_level) {
        if (ftwbuf->level > old_level) {
            strncpy(old_dir, path, strlen(path) - strlen(strrchr(path, '/')));
            old_dir[strlen(path) - strlen(strrchr(path, '/'))] = '\0';
            strcat(pasted_dir, strrchr(old_dir, '/'));
        } else {
            pasted_dir[strlen(pasted_dir) - strlen(strrchr(pasted_dir, '/')) + 1]= '\0';
        }
        old_level = ftwbuf->level;
    }
    strcpy(pasted_file, pasted_dir);
    strcat(pasted_file, strrchr(path, '/'));
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

/*
 * First i check if ps.cont paths are in the same folder as pasted_dir, and if yes, printed[i] will mark
 * that "i" tab was already updated.
 * Then for each file i see if we are in the same folder where it was copied, and if that's the case,
 * i update that tab too.
 * If tmp->cut == 1, i'll have to remove that file.
 */
void check_pasted(void)
{
    int i, printed[cont];
    file_list *tmp = selected_files;
    if (search_mode == 0) {
        for (i = 0; i < cont; i++) {
            if (strcmp(pasted_dir, ps[i].my_cwd) == 0) {
                list_everything(i, 0, dim - 2, 1, 1);
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
        if ((tmp->cut == 1) || (tmp->cut == -1)) {
                for (i = 0; i < cont; i++) {
                    if ((printed[i] == 0) && (strcmp(tmp->dir, ps[i].my_cwd) == 0))
                        list_everything(i, 0, dim - 2, 1, 1);
                }
            }
            tmp = tmp->next;
        }
        chdir(ps[active].my_cwd);
    }
    print_info("Every files has been copied/moved.", INFO_LINE);
    free_copied_list(selected_files);
    selected_files = NULL;
    memset(pasted_dir, 0, strlen(pasted_dir));
}

void rename_file_folders(void)
{
    char *mesg = "Insert new name:> ", str[PATH_MAX];
    if (file_isCopied())
        return;
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, str);
    noecho();
    if (rename(ps[active].namelist[ps[active].current_position]->d_name, str) == - 1) {
        print_info(strerror(errno), ERR_LINE);
    } else {
        sync_changes();
        print_info("File renamed.", INFO_LINE);
    }
}

void create_dir(void)
{
    char *mesg = "Insert new folder name:> ", str[80];
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

void search(void)
{
    char *mesg = "Insert filename to be found, at least 3 chars:> ";
    int i = 0;
    echo();
    print_info(mesg, INFO_LINE);
    wgetstr(info_win, searched_string);
    noecho();
    if (strlen(searched_string) < 3) {
        print_info(NULL, INFO_LINE);
        return;
    }
    search_file(ps[active].my_cwd);
    if (!found_searched[i]) {
        print_info("No files found.", INFO_LINE);
        return;
    }
    wclear(ps[active].file_manager);
    wborder(ps[active].file_manager, '|', '|', '-', '-', '+', '+', '+', '+');
    mvwprintw(ps[active].file_manager, 0, 0, "Found file searching %s: ", searched_string);
    wattron(ps[active].file_manager, A_BOLD);
    for (i = 0; (i < dim - 2) && (found_searched[i]); i++)
        mvwprintw(ps[active].file_manager, INITIAL_POSITION + i, 4, "%s", found_searched[i]);
    wattroff(ps[active].file_manager, A_BOLD);
    while (found_searched[i])
        i++;
    search_loop(i);
    for (i = 0; found_searched[i]; i++) {
        found_searched[i] = realloc(found_searched[i], 0);
        free(found_searched[i]);
    }
}

static void search_loop(int size)
{
    char *str = NULL, *mesg = "Open file? y to open, n to switch to the folder";
    int c;
    search_mode = 1;
    ps[active].delta = 0;
    ps[active].current_position = 0;
    ps[active].number_of_files = size;
    print_info("q to leave search win", INFO_LINE);
    mvwprintw(ps[active].file_manager, INITIAL_POSITION, 1, "->");
    wattron(ps[active].file_manager, A_BOLD);
    do {
        c = wgetch(ps[active].file_manager);

        switch (c) {
            case KEY_UP:
                scroll_up(found_searched[ps[active].current_position]);
                break;
            case KEY_DOWN:
                scroll_down(found_searched[ps[active].current_position]);
                break;
            case 10:
                str = strrchr(found_searched[ps[active].current_position], '/');
                if ((strlen(str) != 1) && (ask_user(mesg) == 1)) {
                    manage_file(found_searched[ps[active].current_position]);
                } else {
                    found_searched[ps[active].current_position][strlen(found_searched[ps[active].current_position]) - strlen(str)] = '\0';
                    c = 'q';
                    strcpy(ps[active].my_cwd, found_searched[ps[active].current_position]);
                }
                break;
        }
    } while (c != 'q');
    wattroff(ps[active].file_manager, A_BOLD);
    search_mode = 0;
    change_dir(ps[active].my_cwd);
}

void print_support(char *str)
{
    pthread_t print_thread;
    char *mesg = "Do you really want to print this file? y/n:> ";
    if (ask_user(mesg) == 1) {
        if (access("/usr/bin/lp", F_OK ) != -1) {
            pthread_create(&print_thread, NULL, print_file, str);
            pthread_detach(print_thread);
        } else {
            print_info("You must have cups installed.", ERR_LINE);
        }
    }
}

static void *print_file(void *filename)
{
    cupsPrintFile(cupsGetDefault(), (char *)filename, "ncursesFM job", 0, NULL);
    return NULL;
}

void create_archive(void)
{
    char *mesg = "Insert new file name:> ", archive_path[PATH_MAX], str[PATH_MAX];
    if ((selected_files) && (ask_user("Do you really want to compress these files?") == 1)) {
        archive = archive_write_new();
        archive_write_add_filter_gzip(archive);
        archive_write_set_format_pax_restricted(archive);
        echo();
        print_info(mesg, INFO_LINE);
        wgetstr(info_win, str);
        strcpy(archive_path, ps[active].my_cwd);
        strcat(archive_path, "/");
        strcat(archive_path, str);
        noecho();
        archive_write_open_filename(archive, archive_path);
        pthread_create(&th, NULL, archiver_func, (void *)archive_path);
    }
}

static void *archiver_func(void *archive_path)
{
    file_list *tmp = selected_files;
    char str[PATH_MAX];
    int i;
    while (tmp) {
        nftw(tmp->name, recursive_compress, 64, FTW_MOUNT | FTW_PHYS);
        tmp = tmp->next;
    }
    archive_write_free(archive);
    archive = NULL;
    if (search_mode == 0) {
        strcpy(str, archive_path);
        str[strlen(str) - strlen(strrchr(str, '/'))] = '\0';
        for (i = 0; i < cont; i++) {
            if (strcmp(str, ps[i].my_cwd) == 0)
                list_everything(i, 0, dim - 2, 1, 1);
        }
        chdir(ps[active].my_cwd);
    }
    print_info("The archive is ready.", INFO_LINE);
    free_copied_list(selected_files);
    selected_files = NULL;
    return NULL;
}

static int recursive_compress(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    char buff[8192];
    int len, fd;
    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, path);
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
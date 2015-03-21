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

void change_dir(char *str)
{
    chdir(str);
    getcwd(ps.my_cwd[ps.active], PATH_MAX);
    list_everything(ps.active, 0, dim - 2, 1, 1);
    clear_info(INFO_LINE);
    clear_info(ERR_LINE);
}

void switch_hidden(void)
{
    int i;
    config.show_hidden = 1 - config.show_hidden;
    for (i = 0; i < ps.cont; i++)
        list_everything(i, 0, dim - 2, 1, 1);
}

void manage_file(char *str)
{
    char *ext;
    if (file_isCopied())
        return;
    ext = strrchr(str, '.');
    if ((ext) && (isIso(ext))) {
        if ((config.iso_mount_point) && (access(config.iso_mount_point, W_OK) != -1))
            iso_mount_service(str);
        else
            print_info("You have to specify an iso mount point in config file and you must have write permissions there.", ERR_LINE);
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

static void iso_mount_service(char *str)
{
    pid_t pid;
    char mount_point[strlen(str) - 4 + strlen(config.iso_mount_point)];
    strcpy(mount_point, config.iso_mount_point);
    strcat(mount_point, "/");
    strncat(mount_point, str, strlen(str) - 4);
    if (access("/usr/bin/fuseiso", F_OK) != -1) {
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
        list_everything(ps.active, 0, dim - 2, 1, 1);
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
        if (rmrf(namelist[ps.active][ps.current_position[ps.active]]->d_name) == -1)
            print_info("Could not remove. Check user permissions.", ERR_LINE);
        else {
            list_everything(ps.active, 0, dim - 2, 1, 1);
            sync_changes();
            print_info("File/dir removed.", INFO_LINE);
        }
    }
}

void manage_c_press(char c)
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

void paste_file(void)
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

void check_pasted(void)
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


void rename_file_folders(void)
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

void search(void)
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

void print_support(char *str)
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
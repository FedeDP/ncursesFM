#include "../inc/bookmarks.h"

static void get_xdg_dirs(const char *home_dir);

static int num_bookmarks;
const char *bookmarks_path = "/.config/ncursesFM-bookmarks";

void get_bookmarks(void) {
    FILE *f;
    const char *home_dir = getpwuid(getuid())->pw_dir;
    char fullpath[PATH_MAX + 1];
    
    get_xdg_dirs(home_dir);
    if (num_bookmarks < MAX_BOOKMARKS) {
        sprintf(fullpath, "%s%s", home_dir, bookmarks_path);
        if ((f = fopen(fullpath, "r"))) {
            int i = num_bookmarks;
            while (fscanf(f, "%s", bookmarks[i]) == 1 && i < MAX_BOOKMARKS) {
                i++;
            }
            fclose(f);
            if (i == MAX_BOOKMARKS) {
                WARN("Too many bookmarks. Max 30.");
            }
            num_bookmarks = i;
        }
    }
}

static void get_xdg_dirs(const char *home_dir) {
    int i = 0;
    FILE *f;
    char str[PATH_MAX + 1] = {0};
    char line[1000], file_path[PATH_MAX + 1];
    const char *path = "/.config/user-dirs.dirs";
    
    sprintf(file_path, "%s%s", home_dir, path);
    if ((f = fopen(file_path, "r"))) {
        while (fgets(line, sizeof(line), f) && i < MAX_BOOKMARKS) {
            if (*line == '#') {
                continue;
            }
            strcpy(str, strchr(line, '/') + 1);
            str[strlen(str) - 2] = '\0'; // -1 for newline - 1 for closing Double quotation mark
            sprintf(bookmarks[i], "%s/%s", home_dir, str);
            i++;
        }
        fclose(f);
        if (i == MAX_BOOKMARKS) {
            WARN("Too many bookmarks. Max 30.");
        }
        num_bookmarks = i;
    }
}

void add_file_to_bookmarks(const char *str) {
    FILE *f;
    const char *home_dir = getpwuid(getuid())->pw_dir;
    char fullpath[PATH_MAX + 1], c;
    
    ask_user("Adding current file to bookmarks. Proceed? Y/n:> ", &c, 1, 'y');
    if (c == 'n' || quit) {
        return;
    }
    sprintf(fullpath, "%s%s", home_dir, bookmarks_path);
    if ((f = fopen(fullpath, "a+"))) {
        fprintf(f, "%s\n", str);
        fclose(f);
        print_info("Added to bookmarks!", INFO_LINE);
        if (num_bookmarks < MAX_BOOKMARKS) {
            strcpy(bookmarks[num_bookmarks], str);
            num_bookmarks++;
        }
    }
}

void show_bookmarks(void) {
    if (num_bookmarks) {
        pthread_mutex_lock(&fm_lock[active]);
        ps[active].number_of_files = num_bookmarks;
        str_ptr[active] = bookmarks;
        bookmarks_mode[active] = 1;
        special_mode[active] = 1;
        sprintf(ps[active].title, bookmarks_mode_str);
        reset_win(active);
        pthread_mutex_unlock(&fm_lock[active]);
    } else {
        print_info("No bookmarks found.", INFO_LINE);
    }
}

void leave_bookmarks_mode(void) {
    pthread_mutex_lock(&fm_lock[active]);
    bookmarks_mode[active] = 0;
    special_mode[active] = 0;
    pthread_mutex_unlock(&fm_lock[active]);
}
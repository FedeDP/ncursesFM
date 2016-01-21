#include "../inc/bookmarks.h"

static void get_xdg_dirs(const char *home_dir);
static void remove_bookmark(void);
static void update_bookmarks_tabs(void);

static int num_bookmarks, xdg_bookmarks;
static const char *bookmarks_path = "/.config/ncursesFM-bookmarks";
static char (bookmarks[MAX_BOOKMARKS])[PATH_MAX + 1];

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
                WARN(too_many_bookmarks);
            }
            num_bookmarks = i;
        } else {
            print_info(bookmarks_file_err, ERR_LINE);
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
            WARN(too_many_bookmarks);
        }
        num_bookmarks = i;
        xdg_bookmarks = i;
    }
}

void add_file_to_bookmarks(const char *str) {
    FILE *f;
    const char *home_dir = getpwuid(getuid())->pw_dir;
    char fullpath[PATH_MAX + 1], c;

    ask_user(bookmarks_add_quest, &c, 1, 'y');
    if (c == 'n' || quit) {
        return;
    }
    sprintf(fullpath, "%s%s", home_dir, bookmarks_path);
    if ((f = fopen(fullpath, "a+"))) {
        fprintf(f, "%s\n", str);
        fclose(f);
        print_info(bookmark_added, INFO_LINE);
        if (num_bookmarks < MAX_BOOKMARKS) {
            strcpy(bookmarks[num_bookmarks], str);
            num_bookmarks++;
            update_bookmarks_tabs();
        } else {
            print_info(too_many_bookmarks, ERR_LINE);
        }
    } else {
        print_info(bookmarks_file_err, ERR_LINE);
    }
}

void remove_bookmark_from_file(void) {
    char c;

    if (ps[active].curr_pos < xdg_bookmarks) {
        print_info(bookmarks_xdg_err, ERR_LINE);
    } else {
        ask_user(bookmarks_rm_quest, &c, 1, 'y');
        if (c == 'n' || quit) {
            return;
        }
        remove_bookmark();
    }
}

static void remove_bookmark(void) {
    FILE *f;
    const char *home_dir = getpwuid(getuid())->pw_dir;
    char fullpath[PATH_MAX + 1];
    int i = ps[active].curr_pos;
    
    sprintf(fullpath, "%s%s", home_dir, bookmarks_path);
    if ((f = fopen(fullpath, "w"))) {
        memmove(&bookmarks[i], &bookmarks[i + 1], (num_bookmarks - 1 - i) * sizeof(bookmarks[0]));
        num_bookmarks--;
        print_info(bookmarks_rm, INFO_LINE);
        for (int i = xdg_bookmarks; i < num_bookmarks; i++) {
            fprintf(f, "%s\n", bookmarks[i]);
        }
        fclose(f);
        update_bookmarks_tabs();
    } else {
        print_info(bookmarks_file_err, ERR_LINE);
    }
}

static void update_bookmarks_tabs(void) {
    for (int i = 0; i < cont; i++) {
        if (bookmarks_mode[i]) {
            update_special_mode(num_bookmarks, i, bookmarks);
        }
    }
}

void show_bookmarks(void) {
    if (num_bookmarks) {
        bookmarks_mode[active] = 1;
        show_special_tab(num_bookmarks, bookmarks, bookmarks_mode_str);
    } else {
        print_info(no_bookmarks, INFO_LINE);
    }
}

void manage_enter_bookmarks(void) {
    char c;
    
    if (change_dir(str_ptr[active][ps[active].curr_pos], active) == -1) {
        update_special_mode(num_bookmarks, active, NULL);
        ask_user(inexistent_bookmark, &c, 1, 'y');
        if (c != 'n') {
            remove_bookmark();
        }
    } else {
        bookmarks_mode[active] = 0;
    }
}

void leave_bookmarks_mode(void) {
    bookmarks_mode[active] = 0;
    change_dir(ps[active].my_cwd, active);
}
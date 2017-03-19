#include "../inc/bookmarks.h"

static void get_xdg_dirs(void);
static void remove_bookmark(int idx);

static int num_bookmarks, xdg_bookmarks;
static char home_dir[PATH_MAX + 1];
static char fullpath[PATH_MAX + 1];
static char (*bookmarks)[PATH_MAX + 1];

void get_bookmarks(void) {
    FILE *f;
    const char *bookmarks_file = "ncursesFM-bookmarks";
    
    strncpy(home_dir, getpwuid(getuid())->pw_dir, PATH_MAX);
    
    if (getenv("XDG_CONFIG_HOME")) {
        snprintf(fullpath, PATH_MAX, "%s/%s", getenv("XDG_CONFIG_HOME"), bookmarks_file);
    } else {
        snprintf(fullpath, PATH_MAX, "%s/.config/%s", home_dir, bookmarks_file);
    }
    get_xdg_dirs();
    if ((f = fopen(fullpath, "r"))) {
        char str[PATH_MAX + 1] = {0};
        
        while (fgets(str, PATH_MAX, f)) {
            bookmarks = safe_realloc(++num_bookmarks, bookmarks);
            strncpy(bookmarks[num_bookmarks - 1], str, PATH_MAX);
        }
        fclose(f);
    } else {
        WARN(bookmarks_file_err);
    }
}

static void get_xdg_dirs(void) {
    FILE *f;
    char line[1000], file_path[PATH_MAX + 1] = {0};
    
    if (getenv("XDG_CONFIG_HOME")) {
        snprintf(file_path, PATH_MAX, "%s/.user-dirs.dirs", getenv("XDG_CONFIG_HOME"));
    } else {
        snprintf(file_path, PATH_MAX, "%s/.config/user-dirs.dirs", home_dir);
    }

    if ((f = fopen(file_path, "r"))) {
        char str[PATH_MAX + 1] = {0};
        
        while (fgets(line, sizeof(line), f)) {
            // avoid comments
            if (*line == '#') {
                continue;
            }
            strncpy(str, strchr(line, '/') + 1, PATH_MAX);
            str[strlen(str) - 2] = '\0'; // -1 for newline - 1 for closing Double quotation mark
            bookmarks = safe_realloc(++num_bookmarks, bookmarks);
            snprintf(bookmarks[num_bookmarks - 1], PATH_MAX, "%s/%s", home_dir, str);
        }
        fclose(f);
        xdg_bookmarks = num_bookmarks;
    }
}

void add_file_to_bookmarks(const char *str) {
    FILE *f;
    char c;

    int present = is_present(str, bookmarks, num_bookmarks, -1, 0);
    if (present != -1) {
        if (config.safe == FULL_SAFE) {
            ask_user(_(bookmark_already_present), &c, 1);
            if (c == _(no)[0] || c == 27) {
                return;
            }
        }
        return remove_bookmark(present);
    }
    if (config.safe == FULL_SAFE) {
        ask_user(_(bookmarks_add_quest), &c, 1);
        if (c == _(no)[0] || c == 27) {
            return;
        }
    }
    if ((f = fopen(fullpath, "a+"))) {
        fprintf(f, "%s\n", str);
        fclose(f);
        print_info(_(bookmark_added), INFO_LINE);
        bookmarks = safe_realloc(++num_bookmarks, bookmarks);
        strncpy(bookmarks[num_bookmarks - 1], str, PATH_MAX);
        update_special_mode(num_bookmarks, bookmarks, bookmarks_);
    } else {
        print_info(_(bookmarks_file_err), ERR_LINE);
    }
}

void remove_bookmark_from_file(void) {
    char c;

    if (ps[active].curr_pos < xdg_bookmarks) {
        print_info(_(bookmarks_xdg_err), ERR_LINE);
    } else {
        if (config.safe == FULL_SAFE) {
            ask_user(_(bookmarks_rm_quest), &c, 1);
            if (c ==  _(no)[0] || c == 27) {
                return;
            }
        }
        remove_bookmark(ps[active].curr_pos);
    }
}

static void remove_bookmark(int idx) {
    FILE *f;
    
    if ((f = fopen(fullpath, "w"))) {
        bookmarks = remove_from_list(&num_bookmarks, bookmarks, idx);
        print_info(_(bookmarks_rm), INFO_LINE);
        for (idx = xdg_bookmarks; idx < num_bookmarks; idx++) {
            fprintf(f, "%s\n", bookmarks[idx]);
        }
        fclose(f);
        update_special_mode(num_bookmarks, bookmarks, bookmarks_);
    } else {
        print_info(_(bookmarks_file_err), ERR_LINE);
    }
}

void show_bookmarks(void) {
    if (num_bookmarks) {
        show_special_tab(num_bookmarks, bookmarks, bookmarks_mode_str, bookmarks_);
    } else {
        print_info(_(no_bookmarks), INFO_LINE);
    }
}

void manage_enter_bookmarks(struct stat current_file_stat) {
    char c;
    
    if (access(str_ptr[active][ps[active].curr_pos], F_OK ) != -1 ) {
        leave_mode_helper(current_file_stat);
    } else {
        if (config.safe == FULL_SAFE) {
            ask_user(_(inexistent_bookmark_quest), &c, 1);
            if (c == _(no)[0] || c == 27) {
                return;
            }
        }
        remove_bookmark(ps[active].curr_pos);
        print_info(_(inexistent_bookmark), INFO_LINE);
    }
}

void remove_all_user_bookmarks(void) {
    for (int i = num_bookmarks - 1; i >= xdg_bookmarks; i--) {
        remove_bookmark(i);
    }
    print_info(_(bookmarks_cleared), INFO_LINE);
}

void free_bookmarks(void) {
    if (bookmarks) {
        free(bookmarks);
    }
}

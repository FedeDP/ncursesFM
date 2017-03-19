#include "../inc/utils.h"

void *remove_from_list(int *num, char (*str)[PATH_MAX + 1], int i) {
    memmove(&str[i], &str[i + 1], ((*num) - 1 - i) * (PATH_MAX + 1));
    return safe_realloc(--(*num), str);
}

void *safe_realloc(const size_t size, char (*str)[PATH_MAX + 1]) {
    if (!(str = realloc(str, (PATH_MAX + 1) * size)) && size) {
        quit = MEM_ERR_QUIT;
        ERROR("could not realloc. Leaving.");
    }
    return str;
}

/*
 * Check if filename has "." in it (otherwise surely it has not extension)
 * Then for each extension in *ext[], check if last strlen(ext[i]) chars of filename are 
 * equals to ext[i].
 */
int is_ext(const char *filename, const char *ext[], int size) {
    int len = strlen(filename);
    
    if (strrchr(filename, '.')) {
        int i = 0;
        
        while (i < size) {
            if (!strcmp(filename + len - strlen(ext[i]), ext[i])) {
                return 1;
            }
            i++;
        }
    }
    return 0;
}

int get_mimetype(const char *path, const char *test) {
    int ret = 0;
    const char *mimetype;
    magic_t magic;
    
    if ((magic = magic_open(MAGIC_MIME_TYPE)) == NULL) {
        ERROR("An error occurred while loading libmagic database.");
        return ret;
    }
    if (magic_load(magic, NULL) == -1) {
        ERROR("An error occurred while loading libmagic database.");
        goto end;
    }
    if ((mimetype = magic_file(magic, path)) == NULL) {
        ERROR("An error occurred while loading libmagic database.");
        goto end;
    }
    if (strstr(mimetype, test)) {
        ret = 1;
    }
    
end:
    magic_close(magic);
    return ret;
}

int move_cursor_to_file(int start_idx, const char *filename, int win) {
    int len;
    char fullpath[PATH_MAX + 1] = {0};
    
    snprintf(fullpath, PATH_MAX, "%s/%s", ps[win].my_cwd, filename);
    len = strlen(fullpath);
    int i = is_present(fullpath, ps[win].nl, ps[win].number_of_files, len, start_idx);
    if (i != -1) {
        if (i != ps[win].curr_pos) {
            void (*f)(int, int);
            int delta;
            
            if (i < ps[win].curr_pos) {
                f = scroll_up;
                delta = ps[win].curr_pos - i;
            } else {
                f = scroll_down;
                delta = i - ps[win].curr_pos;
            }
            f(win, delta);
        }
        return 1;
    }
    return 0;
}

void save_old_pos(int win) {
    char *str;
    
    str = strrchr(ps[win].nl[ps[win].curr_pos], '/') + 1;
    strncpy(ps[win].old_file, str, NAME_MAX);
}

int is_present(const char *name, char (*str)[PATH_MAX + 1], int num, int len, int start_idx) {
    int cmp;
    
    for (int i = start_idx; i < num; i++) {
        if (len != -1) {
            cmp = strncmp(str[i], name, len);
        } else {
            cmp = strcmp(str[i], name);
        }
        if (!cmp) {
            return i;
        }
    }
    return -1;
}

/*
 * Helper function used in show_stat: received a size,
 * it changes the unit from Kb to Mb to Gb if size > 1024(previous unit)
 */
void change_unit(float size, char *str) {
    char *unit[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    
    while ((size > 1024) && (i < NUM(unit) - 1)) {
        size /= 1024;
        i++;
    }
    sprintf(str, "%.2f%s", size, unit[i]);
}

void leave_mode_helper(struct stat s) {
    char str[PATH_MAX + 1] = {0};
    
    strncpy(str, str_ptr[active][ps[active].curr_pos], PATH_MAX);
    if (!S_ISDIR(s.st_mode)) {
        strncpy(ps[active].old_file, strrchr(str_ptr[active][ps[active].curr_pos], '/') + 1, NAME_MAX);
        int len = strlen(str_ptr[active][ps[active].curr_pos]) - strlen(ps[active].old_file);
        str[len] = '\0';
    } else {
        memset(ps[active].old_file, 0, strlen(ps[active].old_file));
    }
    leave_special_mode(str, active);
}

#include "../inc/search.h"

static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int search_inside_archive(const char *path);
static void *search_thread(void *x);

void search(void) {
    pthread_t search_th;
    char c;

    ask_user(search_insert_name, sv.searched_string, 20, 0);
    if (strlen(sv.searched_string) < 5) {
        print_info(searched_string_minimum, INFO_LINE);
        return;
    }
    sv.found_cont = 0;
    sv.search_archive = 0;
    ask_user(search_archives, &c, 1, 'n');
    if (c == 'y') {
        sv.search_archive = 1;
    }
    sv.searching = 1;
    print_info(NULL, INFO_LINE);
    pthread_create(&search_th, NULL, search_thread, NULL);
    pthread_detach(search_th);
}

static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    char fixed_str[NAME_MAX];
    int len, ret = 0;

    if (ftwbuf->level == 0) {
        return 0;
    }
    strcpy(fixed_str, strrchr(path, '/') + 1);
    if ((sv.search_archive) && (is_ext(fixed_str, arch_ext, NUM(arch_ext)))) {
        return search_inside_archive(path);
    }
    len = strlen(sv.searched_string);
    if (strncmp(fixed_str, sv.searched_string, len) == 0) {
        strcpy(sv.found_searched[sv.found_cont], path);
        if (typeflag == FTW_D) {
            strcat(sv.found_searched[sv.found_cont], "/");
        }
        sv.found_cont++;
        if (sv.found_cont == MAX_NUMBER_OF_FOUND) {
            ret = 1;
        }
    }
    return ret;
}

static int search_inside_archive(const char *path) {
    char str[PATH_MAX], *ptr;
    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    int len;

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    if ((a) && (archive_read_open_filename(a, path, BUFF_SIZE) == ARCHIVE_OK)) {
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            strcpy(str, archive_entry_pathname(entry));
            len = strlen(str);
            if (str[len - 1] == '/') {  // check if we're on a dir
                str[len - 1] = '\0';
            }
            ptr = str;
            if (strrchr(str, '/')) {
                ptr = strrchr(str, '/') + 1;
            }
            len = strlen(sv.searched_string);
            if (strncmp(ptr, sv.searched_string, len) == 0) {
                sprintf(sv.found_searched[sv.found_cont], "%s/%s", path, archive_entry_pathname(entry));
                sv.found_cont++;
            }
        }
    }
    archive_read_free(a);
    return 0;
}

static void *search_thread(void *x) {
    nftw(ps[active].my_cwd, recursive_search, 64, FTW_MOUNT | FTW_PHYS);
    if ((sv.found_cont == MAX_NUMBER_OF_FOUND) || (sv.found_cont == 0)) {
        sv.searching = 0;
        if (sv.found_cont == MAX_NUMBER_OF_FOUND) {
            print_info(too_many_found, INFO_LINE);
        } else {
            print_info(no_found, INFO_LINE);
        }
    } else {
        sv.searching = 2;
        print_info(NULL, INFO_LINE);
    }
    return NULL;
}

void list_found(void) {
    sv.searching = 3 + active;
    ps[active].number_of_files = sv.found_cont;
    str_ptr[active] = sv.found_searched;
    reset_win(active);
    sprintf(ps[active].title, "Found file searching %s:", sv.searched_string);
    list_everything(active, 0, 0);
    print_info(NULL, INFO_LINE);
}

void leave_search_mode(const char *str) {
    sv.searching = 0;
    print_info(NULL, INFO_LINE);
    change_dir(str);
}

/*
 * While in search mode, enter will switch to current highlighted file's dir.
 * It checks if file is inside an archive; if so, it removes last part
 * of file's path while we're inside an archive.
 * Then returns the index where the real filename begins (to extract the directory path).
 */
int search_enter_press(const char *str) {
    char arch_str[PATH_MAX] = {0};
    const char *tmp;
    int len;

    if (sv.search_archive) {
        strcpy(arch_str, str);
        while ((len = strlen(arch_str))) {
            tmp = strrchr(arch_str, '/');
            if (is_ext(tmp, arch_ext, NUM(arch_ext))) {
                break;
            }
            arch_str[len - strlen(tmp)] = '\0';
        }
    }
    if (strlen(arch_str)) {
        tmp = arch_str;
    } else {
        tmp = str;
    }
    return (strlen(tmp) - strlen(strrchr(tmp, '/')));
}
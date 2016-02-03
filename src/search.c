#include "../inc/search.h"

static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static int search_inside_archive(const char *path);
static void *search_thread(void *x);

void search(void) {
    char c;

    ask_user(search_insert_name, sv.searched_string, 20, 0);
    if (quit || strlen(sv.searched_string) < 5) {
        print_info(searched_string_minimum, ERR_LINE);
    } else {
        sv.found_cont = 0;
        sv.search_archive = 0;
        sv.search_lazy = 0;
        ask_user(search_archives, &c, 1, 'n');
        if (quit) {
            return;
        }
        if (c == 'y') {
            sv.search_archive = 1;
        }
        ask_user(lazy_search, &c, 1, 'n');
        if (quit) {
            return;
        }
        if (c == 'y') {
            sv.search_lazy = 1;
        }
        sv.searching = 1;
        print_info("", SEARCH_LINE);
        pthread_create(&search_th, NULL, search_thread, NULL);
    }
}

static int recursive_search(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    char *fixed_str;
    int len, r = 0, ret = 0;

    if (ftwbuf->level == 0) {
        return 0;
    }
    fixed_str = strrchr(path, '/') + 1;
    if ((sv.search_archive) && (is_ext(fixed_str, arch_ext, NUM(arch_ext)))) {
        return search_inside_archive(path);
    }
    len = strlen(sv.searched_string);
    if (!sv.search_lazy) {
        r = !strncmp(fixed_str, sv.searched_string, len);
    } else if (strcasestr(fixed_str, sv.searched_string)) {
        r = 1;
    }
    if (r) {
        strcpy(sv.found_searched[sv.found_cont], path);
        if (typeflag == FTW_D) {
            strcat(sv.found_searched[sv.found_cont], "/");
        }
        sv.found_cont++;
        if (sv.found_cont == MAX_NUMBER_OF_FOUND) {
            ret = 1;
        }
    }
    return quit ? 1 : ret;
}

/*
 * For each entry in the archive, it checks "entry + len" pointer against searched string.
 * Len is always the offset of the current dir inside archive, eg: foo.tgz/bar/x,
 * while checking x, len will be strlen("bar/")
 */
static int search_inside_archive(const char *path) {
    char *ptr;
    int len = 0, ret = 0, r = 0, string_length;
    struct archive_entry *entry;
    struct archive *a = archive_read_new();

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    string_length = strlen(sv.searched_string);
    if ((a) && (archive_read_open_filename(a, path, BUFF_SIZE) == ARCHIVE_OK)) {
        while ((!quit) && (!ret) && (archive_read_next_header(a, &entry) == ARCHIVE_OK)) {
            if (!sv.search_lazy) {
                r = !strncmp(archive_entry_pathname(entry) + len, sv.searched_string, string_length);
            } else if (strcasestr(archive_entry_pathname(entry) + len, sv.searched_string)) {
                r = 1;
            }
            if (r) {
                sprintf(sv.found_searched[sv.found_cont], "%s/%s", path, archive_entry_pathname(entry));
                sv.found_cont++;
                if (sv.found_cont == MAX_NUMBER_OF_FOUND) {
                    ret = 1;
                }
            }
            ptr = strrchr(archive_entry_pathname(entry), '/');
            if ((ptr) && (strlen(ptr) == 1)) {
                len = strlen(archive_entry_pathname(entry));
            }
        }
    }
    archive_read_free(a);
    return quit ? 1 : ret;
}

static void *search_thread(void *x) {
    INFO("starting recursive search...");
    nftw(ps[active].my_cwd, recursive_search, 64, FTW_MOUNT | FTW_PHYS);
    INFO("ended recursive search");
    if (!quit) {
        if ((sv.found_cont == MAX_NUMBER_OF_FOUND) || (sv.found_cont == 0)) {
            sv.searching = 0;
            if (sv.found_cont == MAX_NUMBER_OF_FOUND) {
                print_info(too_many_found, INFO_LINE);
            } else {
                print_info(no_found, INFO_LINE);
            }
        } else {
            sv.searching = 2;
        }
        print_info("", SEARCH_LINE);
    }
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

void list_found(void) {
    char str[100];

    sprintf(str, "Files found searching %s:", sv.searched_string);
    sv.searching = 3 + active;
    sprintf(searching_mess[sv.searching - 1], "%d files found.", sv.found_cont);
    show_special_tab(sv.found_cont, sv.found_searched, str);
    print_info("", SEARCH_LINE);
}

void leave_search_mode(const char *str) {
    sv.searching = 0;
    print_info("", SEARCH_LINE);
    change_dir(str, active);
}

/*
 * While in search mode, enter will switch to current highlighted file's dir.
 * It checks if file is inside an archive; if so, it removes last part
 * of file's path while we're inside an archive.
 * Then returns the index where the real filename begins (to extract the directory path).
 */
int search_enter_press(const char *str) {
    char arch_str[PATH_MAX + 1] = {0};
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
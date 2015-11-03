#include "../inc/archiver.h"

static void archiver_func(void);
static int recursive_archive(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void extractor_thread(struct archive *a);

static struct archive *archive;

int create_archive(void) {
    archive = archive_write_new();
    if (((archive_write_add_filter_gzip(archive) == ARCHIVE_FATAL) || (archive_write_set_format_pax_restricted(archive) == ARCHIVE_FATAL)) ||
        (archive_write_open_filename(archive, thread_h->filename) == ARCHIVE_FATAL)) {
        archive_write_free(archive);
        archive = NULL;
        return -1;
    }
    archiver_func();
    return 0;
}

static void archiver_func(void) {
    file_list *tmp = thread_h->selected_files;
    char *str;

    while (tmp) {
        str = strrchr(tmp->name, '/');
        distance_from_root = strlen(tmp->name) - strlen(str);
        nftw(tmp->name, recursive_archive, 64, FTW_MOUNT | FTW_PHYS);
        tmp = tmp->next;
    }
    archive_write_free(archive);
    archive = NULL;
}

static int recursive_archive(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    char buff[BUFF_SIZE], entry_name[NAME_MAX];
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

int try_extractor(void) {
    struct archive *a;

    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    if ((a) && (archive_read_open_filename(a, thread_h->filename, BUFF_SIZE) == ARCHIVE_OK)) {
        extractor_thread(a);
        return 0;
    }
    archive_read_free(a);
    return -1;
}

static void extractor_thread(struct archive *a) {
    struct archive *ext;
    struct archive_entry *entry;
    int len;
    int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
    char buff[BUFF_SIZE], current_dir[PATH_MAX], fullpathname[PATH_MAX];
    char *tmp;

    strcpy(current_dir, thread_h->filename);
    tmp = strrchr(current_dir, '/');
    len = strlen(current_dir) - strlen(tmp);
    current_dir[len] = '\0';
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    while (archive_read_next_header(a, &entry) != ARCHIVE_EOF) {
        sprintf(fullpathname, "%s/%s", current_dir, archive_entry_pathname(entry));
        archive_entry_set_pathname(entry, fullpathname);
        archive_write_header(ext, entry);
        len = archive_read_data(a, buff, sizeof(buff));
        while (len > 0) {
            archive_write_data(ext, buff, len);
            len = archive_read_data(a, buff, sizeof(buff));
        }
    }
    archive_read_free(a);
    archive_write_free(ext);
}
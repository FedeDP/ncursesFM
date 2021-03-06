#include "../inc/archiver.h"

static void archiver_func(void);
static int recursive_archive(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
#if ARCHIVE_VERSION_NUMBER >= 3002000
static const char *passphrase_callback(struct archive *a, void *_client_data);
#endif
static int try_extractor(const char *tmp);
static void extractor_thread(struct archive *a, const char *current_dir);

static struct archive *archive;
static int distance_from_root;

/*
 * It tries to create a new archive to write inside it,
 * it fails if it cannot add the proper filter, or cannot set proper format, or
 * if it cannot open thread_h->full_path (ie, the desired pathname of the new archive)
 */
int create_archive(void) {
    archive = archive_write_new();
    if ((archive_write_add_filter_gzip(archive) == ARCHIVE_OK) &&
        (archive_write_set_format_pax_restricted(archive) == ARCHIVE_OK) &&
        (archive_write_open_filename(archive, thread_h->full_path) == ARCHIVE_OK)) {
        archiver_func();
        return 0;
    }
    ERROR(archive_error_string(archive));
    archive_write_free(archive);
    archive = NULL;
    return -1;
}

/*
 * For each of the selected files, calculates the distance from root and calls nftw with recursive_archive.
 * Example: archiving /home/me/Scripts/ folder -> it contains {/x.sh, /foo/bar}.
 * recursive_archive has to create the entry exactly like /desired/path/name.tgz/{x.sh, foo/bar}
 * it copies as entry_name the pointer to current path + distance_from_root + 1, in our case:
 * path is /home/me/Scripts/x.sh and (path + distance_from_root + 1) points exatcly to x.sh.
 * The entry will be written to the new archive, and then data will be copied.
 */
static void archiver_func(void) {
    char path[PATH_MAX + 1] = {0};

    for (int i = 0; i < thread_h->num_selected; i++) {
        strncpy(path, thread_h->selected_files[i], PATH_MAX);
        distance_from_root = strlen(dirname(path));
        nftw(thread_h->selected_files[i], recursive_archive, 64, FTW_MOUNT | FTW_PHYS);
    }
    archive_write_free(archive);
    archive = NULL;
}

static int recursive_archive(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    char entry_name[PATH_MAX + 1] = {0};
    int fd;
    struct archive_entry *entry = archive_entry_new();

    strncpy(entry_name, path + distance_from_root + 1, PATH_MAX);
    archive_entry_set_pathname(entry, entry_name);
    archive_entry_copy_stat(entry, sb);
    archive_write_header(archive, entry);
    archive_entry_free(entry);
    fd = open(path, O_RDONLY);
    if (fd != -1) {
        char buff[BUFF_SIZE] = {0};
        int len;
        
        len = read(fd, buff, sizeof(buff));
        while (len > 0) {
            archive_write_data(archive, buff, len);
            len = read(fd, buff, sizeof(buff));
        }
        close(fd);
    }
    return 0;
}

int extract_file(void) {
    int ret = 0;
    
    for (int i = 0; i < thread_h->num_selected; i++) {
        if (is_ext(thread_h->selected_files[i], arch_ext, NUM(arch_ext))) {
            ret += try_extractor(thread_h->selected_files[i]);
        } else {
            ret--;
        }
    }
    return !ret ? 0 : -1;
}

#if ARCHIVE_VERSION_NUMBER >= 3002000
static const char *passphrase_callback(struct archive *a, void *_client_data) {
    uint64_t u = 1;
    
    if (eventfd_write(archive_cb_fd[0], u) == -1) {
        return NULL;
    }
    if (eventfd_read(archive_cb_fd[1], &u) == -1) {
        return NULL;
    }
    if (quit || passphrase[0] == 27) {
        return NULL;
    }
    return passphrase;
}
#endif

static int try_extractor(const char *tmp) {
    struct archive *a;

    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
#if ARCHIVE_VERSION_NUMBER >= 3002000
    archive_read_set_passphrase_callback(a, NULL, passphrase_callback);
#endif
    if ((a) && (archive_read_open_filename(a, tmp, BUFF_SIZE) == ARCHIVE_OK)) {
        char path[PATH_MAX + 1] = {0};
        
        strncpy(path, tmp, PATH_MAX);
        char *current_dir = dirname(path);
        extractor_thread(a, current_dir);
        return 0;
    }
    archive_read_free(a);
    return -1;
}

/*
 * calculates current_dir path, then creates the write_disk_archive that
 * will read from the selected archives and will write files on disk.
 * While there are headers inside the archive being read, it goes on copying data from
 * the read archive to the disk.
 */
static void extractor_thread(struct archive *a, const char *current_dir) {
    struct archive *ext;
    struct archive_entry *entry;
    int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
    char buff[BUFF_SIZE], fullpathname[PATH_MAX + 1];
    char name[PATH_MAX + 1] = {0}, tmp_name[PATH_MAX + 1] = {0};

    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    while (archive_read_next_header(a, &entry) != ARCHIVE_EOF) {
        strncpy(name, archive_entry_pathname(entry), PATH_MAX);
        int num = 0;
        /* avoid overwriting a file/dir in path if it has the same name of a file being extracted there */
        if (strchr(name, '/')) {
            strncpy(tmp_name, strchr(name, '/'), PATH_MAX);
        } else {
            tmp_name[0] = '\0';
        }
        int len = strlen(name) - strlen(tmp_name);
        while (access(name, F_OK) == 0) {
            num++;
            snprintf(name + len, PATH_MAX - len, "%d%s", num, tmp_name);
        }
        snprintf(fullpathname, PATH_MAX, "%s/%s", current_dir, name);
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

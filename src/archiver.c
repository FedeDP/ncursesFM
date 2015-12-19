#include "../inc/archiver.h"

static void archiver_func(void);
static int recursive_archive(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void extractor_thread(struct archive *a);

static struct archive *archive;

/*
 * It tries to create a new archive for writing inside,
 * it fails if it cannot add the proper filter, or cannot set proper format, or
 * if it cannot open thread_h->filename (ie, the desired pathname of the new archive)
 */
int create_archive(void) {
    archive = archive_write_new();
    if (((archive_write_add_filter_gzip(archive) == ARCHIVE_FATAL) || (archive_write_set_format_pax_restricted(archive) == ARCHIVE_FATAL)) ||
        (archive_write_open_filename(archive, thread_h->filename) != ARCHIVE_OK)) {
        archive_write_free(archive);
        archive = NULL;
        return -1;
    }
    archiver_func();
    return 0;
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
    char buff[BUFF_SIZE], entry_name[PATH_MAX + 1];
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

/*
 * calculates current_dir path, then creates the write_disk_archive that
 * will read from the selected archives and will write files on disk.
 * While there are headers inside the archive being read, it goes on copying data from
 * the read archive to the disk.
 */
static void extractor_thread(struct archive *a) {
    struct archive *ext;
    struct archive_entry *entry;
    int len, num = 0;
    int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
    char buff[BUFF_SIZE], current_dir[PATH_MAX + 1], fullpathname[PATH_MAX + 1];
    char *tmp;
    char name[PATH_MAX + 1], tmp_name[PATH_MAX + 1];

    strcpy(current_dir, thread_h->filename);
    tmp = strrchr(current_dir, '/');
    len = strlen(current_dir) - strlen(tmp);
    current_dir[len] = '\0';
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    while (archive_read_next_header(a, &entry) != ARCHIVE_EOF) {
        strcpy(name, archive_entry_pathname(entry));
        /* only perform overwrite check if on the first entry, or if num != 1 */
        if ((len == strlen(current_dir)) || (num != 1)) {
            /* avoid overwriting a file/dir in path if it has the same name of a file being extracted there */
            strcpy(tmp_name, strchr(name, '/'));
            len = strlen(name) - strlen(tmp_name);
            if (num == 0) {
                while (access(name, F_OK) != -1) {
                    num++;
                    sprintf(name + len, "%d%s", num, tmp_name);
                }
            } else {
                sprintf(name + len, "%d%s", num, tmp_name);
            }
        }
        sprintf(fullpathname, "%s/%s", current_dir, name);
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
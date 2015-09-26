#include "declarations.h"
#include <sys/stat.h>
#include <magic.h>
#include <unistd.h>
#include <signal.h>
#ifdef SYSTEMD_PRESENT
#include <systemd/sd-bus.h>
#endif
#include <mntent.h>

int is_ext(const char *filename, const char *ext[], int size);
void ask_user(const char *str, char *input, int dim, char c);
void print_info(const char *str, int i);
void *safe_malloc(ssize_t size, const char *str);
int get_mimetype(const char *path, const char *test);
void init_thread(int type, int (*f)(void));
void free_copied_list(file_list *h);
int remove_from_list(const char *name);
file_list *select_file(file_list *h, const char *str);
void free_everything(void);
#if defined(LIBUDEV_PRESENT) && (SYSTEMD_PRESENT)
void mount_fs(const char *str);
int is_mounted(const char *dev_path);
#endif
#ifdef SYSTEMD_PRESENT
void *install_package(void *str);
#endif
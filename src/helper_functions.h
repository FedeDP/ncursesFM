#include "declarations.h"
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <signal.h>

#ifdef SYSTEMD_PRESENT
#include <systemd/sd-bus.h>
#include <mntent.h>
#endif

int is_ext(const char *filename, const char *ext[], int size);
void ask_user(const char *str, char *input, int dim, char c);
void print_info(const char *str, int i);
void *safe_malloc(ssize_t size, const char *str);
void init_thread(int type, int (* const f)(void));
void free_copied_list(file_list *h);
int remove_from_list(const char *name);
file_list *select_file(file_list *h, const char *str);
void free_everything(void);
#ifdef SYSTEMD_PRESENT
void mount_fs(const char *str, const char *method, int mount);
int is_mounted(const char *dev_path);
void isomount(const char *str);
void *install_package(void *str);
#endif
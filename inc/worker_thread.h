#include "ui_functions.h"

#ifdef SYSTEMD_PRESENT
#include "inhibit.h"
#endif

void init_thread(int type, int (* const f)(void));
int remove_from_list(const char *name);
file_list *select_file(file_list *h, const char *str);
void free_everything(void);
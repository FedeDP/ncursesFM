#include "quit_functions.h"

void free_copied_list(copied_file_list *h)
{
    if (h->next)
        free_copied_list(h->next);
    free(h);
}

void free_everything(void)
{
    int i, j;
    for (j = 0; j < ps.cont; j++) {
        if (namelist[j] != NULL) {
            for (i = ps.number_of_files[j] - 1; i >= 0; i--)
                free(namelist[j][i]);
            free(namelist[j]);
        }
    }
    free(config.editor);
    free(config.iso_mount_point);
    if (ps.copied_files)
        free_copied_list(ps.copied_files);
}


void quit_func(void)
{
    char *mesg = "A paste job is still running. Do you want to wait for it?(You should!) y/n:> ";
    if ((ps.pasted == - 1) && (ask_user(mesg) == 1))
        pthread_join(th, NULL);
}
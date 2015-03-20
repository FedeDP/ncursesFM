#include "helper_functions.h"

const char *iso_extensions[] = {".iso", ".bin", ".nrg", ".img", ".mdf"};

int isIso(char *ext)
{
    int i = 0;
    while (*(iso_extensions + i)) {
        if (strcmp(ext, *(iso_extensions + i)) == 0)
            return 1;
        i++;
    }
    return 0;
}

int file_isCopied(void)
{
    char full_path_current_position[PATH_MAX];
    copied_file_list *tmp = ps.copied_files;
    get_full_path(full_path_current_position);
    while (tmp) {
        if (strcmp(tmp->copied_file, full_path_current_position) == 0) {
            print_info("The file is already selected for copy. Please cancel the copy before.", ERR_LINE);
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

void get_full_path(char *full_path_current_position)
{
    strcpy(full_path_current_position, ps.my_cwd[ps.active]);
    strcat(full_path_current_position, "/");
    strcat(full_path_current_position,namelist[ps.active][ps.current_position[ps.active]]->d_name);
}

int ask_user(char *str)
{
    char c;
    echo();
    print_info(str, INFO_LINE);
    c = wgetch(info_win);
    noecho();
    clear_info(INFO_LINE);
    if (c == 'y')
        return 1;
    return 0;
}

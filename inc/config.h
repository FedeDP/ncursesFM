#include "utils.h"

#ifdef LIBCONFIG_PRESENT
#include <libconfig.h>
#endif
#include <getopt.h>

void parse_cmd(int argc, char * const argv[]);
#ifdef LIBCONFIG_PRESENT
void check_config_files(void);
void read_config_file(const char *dir);
#endif
void config_checks(void);

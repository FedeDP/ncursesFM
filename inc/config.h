#include "utils.h"

#ifdef LIBCONFIG_PRESENT
#include <libconfig.h>
#endif
#include <getopt.h>

void parse_cmd(int argc, char * const argv[]);
#ifdef LIBCONFIG_PRESENT
void load_config_files(void);
#endif
void config_checks(void);

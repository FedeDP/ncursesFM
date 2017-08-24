#include "../inc/config.h"

#ifdef LIBCONFIG_PRESENT
static void read_config_file(const char *dir);
#endif

void parse_cmd(int argc, char * const argv[]) {
    int idx = 0, opt;
    struct option opts[] =
    {
        {"editor", 1, 0, 0},
        {"starting_dir", 1, 0, 0},
        {"helper_win", 1, 0, 0},
        {"loglevel",  1, 0, 0},
        {"persistent_log",  1, 0, 0},
        {"low_level",  1, 0, 0},
        {"safe",    1, 0, 0},
#ifdef LIBNOTIFY_PRESENT
        {"silent", 1, 0, 0},
#endif
#ifdef SYSTEMD_PRESENT
        {"inhibit",    1, 0, 0},
        {"automount",    1, 0, 0},
#endif
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "", opts, &idx)) != -1) {
        if (optarg) {
            switch (idx) {
            case 0:
                strncpy(config.editor, optarg, PATH_MAX);
                break;
            case 1:
                strncpy(config.starting_dir, optarg, PATH_MAX);
                break;
            case 2:
                config.starting_helper = atoi(optarg);
                break;
            case 3:
                config.loglevel = atoi(optarg);
                break;
            case 4:
                config.persistent_log = atoi(optarg);
                break;
            case 5:
                config.bat_low_level = atoi(optarg);
                break;
            case 6:
                config.safe = atoi(optarg);
                break;
#ifdef LIBNOTIFY_PRESENT
            case 7:
                config.silent = atoi(optarg);
                break;
#endif
#ifdef SYSTEMD_PRESENT
#ifdef LIBNOTIFY_PRESENT
            case 8:
                config.inhibit = atoi(optarg);
                break;
            case 9:
                config.automount = atoi(optarg);
                break;
#else
            case 7:
                config.inhibit = atoi(optarg);
                break;
            case 8:
                config.automount = atoi(optarg);
                break;
#endif
#endif
            }
        }
    }
}

#ifdef LIBCONFIG_PRESENT
void load_config_files(void) {
    char config_path[PATH_MAX + 1] = {0};
    
    // Read global conf file in /etc/default
    read_config_file(CONFDIR);
    
    // Try to get XDG_CONFIG_HOME from env
    if (getenv("XDG_CONFIG_HOME")) {
        strncpy(config_path, getenv("XDG_CONFIG_HOME"), PATH_MAX);
    } else {
        // fallback to ~/.config/
        snprintf(config_path, PATH_MAX, "%s/.config", getpwuid(getuid())->pw_dir);
    }
    read_config_file(config_path);
}

static void read_config_file(const char *dir) {
    config_t cfg;
    char config_file_name[PATH_MAX + 1] = {0};
    const char *str_editor, *str_starting_dir, *str_cursor, *sysinfo;

    snprintf(config_file_name, PATH_MAX, "%s/ncursesFM.conf", dir);
    if (access(config_file_name, F_OK ) == -1) {
        fprintf(stderr, "Config file %s not found.\n", config_file_name);
        return;
    }
    config_init(&cfg);
    if (config_read_file(&cfg, config_file_name) == CONFIG_TRUE) {
        if (config_lookup_string(&cfg, "editor", &str_editor) == CONFIG_TRUE) {
            strncpy(config.editor, str_editor, PATH_MAX);
        }
        config_lookup_int(&cfg, "show_hidden", &config.show_hidden);
        if (config_lookup_string(&cfg, "starting_directory", &str_starting_dir) == CONFIG_TRUE) {
            strncpy(config.starting_dir, str_starting_dir, PATH_MAX);
        }
        config_lookup_int(&cfg, "use_default_starting_dir_second_tab", &config.second_tab_starting_dir);
        config_lookup_int(&cfg, "starting_helper", &config.starting_helper);
#ifdef LIBNOTIFY_PRESENT
        config_lookup_int(&cfg, "silent", &config.silent);
#endif
#ifdef SYSTEMD_PRESENT
        config_lookup_int(&cfg, "inhibit", &config.inhibit);
        config_lookup_int(&cfg, "automount", &config.automount);
#endif
        config_lookup_int(&cfg, "loglevel", &config.loglevel);
        config_lookup_int(&cfg, "persistent_log", &config.persistent_log);
        config_lookup_int(&cfg, "bat_low_level", &config.bat_low_level);
        if (config_lookup_string(&cfg, "cursor_chars", &str_cursor) == CONFIG_TRUE) {
            mbstowcs(config.cursor_chars, str_cursor, 2);
        }
        if (config_lookup_string(&cfg, "sysinfo_layout", &sysinfo) == CONFIG_TRUE) {
            strncpy(config.sysinfo_layout, sysinfo, sizeof(config.sysinfo_layout));
        }
        config_lookup_int(&cfg, "safe", &config.safe);
    } else {
        fprintf(stderr, "Config file: %s at line %d.\n",
                config_error_text(&cfg),
                config_error_line(&cfg));
    }
    config_destroy(&cfg);
}
#endif

void config_checks(void) {
    if ((strlen(config.starting_dir)) && (access(config.starting_dir, F_OK) == -1)) {
        memset(config.starting_dir, 0, strlen(config.starting_dir));
    }
    if (!strlen(config.editor) || (access(config.editor, X_OK) == -1)) {
        memset(config.editor, 0, strlen(config.editor));
        WARN("no editor defined. Trying to get one from env.");
        const char *str;
        
        if ((str = getenv("EDITOR"))) {
            strncpy(config.editor, str, PATH_MAX);
        } else {
            WARN("no editor env var found.");
        }
    }
    if ((config.loglevel < LOG_ERR) || (config.loglevel > NO_LOG)) {
        config.loglevel = LOG_ERR;
    }
    if (config.safe < UNSAFE || config.safe > FULL_SAFE) {
        config.safe = FULL_SAFE;
    }
}

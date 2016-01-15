#include "../inc/time.h"

static void *time_func(void *x);
static void poll_batteries(void);

void start_time(void) {
    INFO("started time thread.");
    pthread_create(&time_th, NULL, time_func, NULL);
}

/*
 * It will monitor time and battery level every 30 sec.
 * As a signal to leave it only needs that program_quit()
 * unlocks its mutex. It will then see that !quit is false
 * and leave.
 */
static void *time_func(void *x) {
    struct timespec absolute_time;
    int ret;

    poll_batteries();
    pthread_mutex_init(&time_lock, NULL);
    clock_gettime(CLOCK_REALTIME, &absolute_time);
    while (!quit) {
        absolute_time.tv_sec += 30;
        ret = pthread_mutex_timedlock(&time_lock, &absolute_time);
        if (!ret || ret == ETIMEDOUT) {
            if (!quit) {
                update_time();
            }
        } else {
            WARN("an error occurred. Leaving.");
            pthread_mutex_unlock(&time_lock);
            break;
        }
    }
    free(batt);
    pthread_mutex_destroy(&time_lock);
    pthread_exit(NULL);
}

static void poll_batteries(void) {
    DIR* d;
    struct dirent* file;
    const char *path = "/sys/class/power_supply/";
    const char *ac_str = "AC";

    if ((d = opendir(path))) {
        while ((file = readdir(d))) {
            if (file->d_name[0] == '.') {
                continue;
            }
            if (!strncmp(file->d_name, ac_str, strlen(ac_str))) {
                sprintf(ac_path, "%s%s", path, file->d_name);
            } else {
                num_of_batt++;
                batt = realloc(batt, sizeof(struct supply) * num_of_batt);
                int i = num_of_batt - 1;
                sprintf(batt[i].path, "%s%s", path, file->d_name);
                if (!query_file(batt[i].path, "energy_full", &(batt[i].energy_full))) {
                    WARN("could not read energy_full from current battery.");
                    batt[i].energy_full = -1;
                }
            }
        }
        closedir(d);
    }
}
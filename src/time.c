#include "../inc/time.h"

static void poll_batteries(void);


static char ac_path[PATH_MAX + 1];
static int num_of_batt, timerfd;
static struct supply *batt;

/*
 * Create 30s timer and returns its fd to
 * the main struct pollfd
 */
int start_timer(void) {
    struct itimerspec timerValue;
    
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd == -1) {
        WARN("could not start timer.");
    } else {
        poll_batteries();
        bzero(&timerValue, sizeof(timerValue));
        timerValue.it_value.tv_sec = 30;
        timerValue.it_value.tv_nsec = 0;
        timerValue.it_interval.tv_sec = 30;
        timerValue.it_interval.tv_nsec = 0;
        timerfd_settime(timerfd, 0, &timerValue, NULL);
        INFO("started time/battery monitor.");
        update_time(ac_path, batt, num_of_batt);
    }
    return timerfd;
}

void timer_func(void) {
    update_time(ac_path, batt, num_of_batt);
}

void free_timer(void) {
    if (batt) {
        free(batt);
    }
    close(timerfd);
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
                batt = realloc(batt, sizeof(struct supply) * (num_of_batt + 1));
                if (batt) {
                    num_of_batt++;
                    int i = num_of_batt - 1;
                    sprintf(batt[i].path, "%s%s", path, file->d_name);
                    if (!query_file(batt[i].path, "energy_full", &(batt[i].energy_full))) {
                        WARN("could not read energy_full from current battery.");
                        batt[i].energy_full = -1;
                    }
                }
            }
        }
        closedir(d);
    }
}
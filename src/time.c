#include "../inc/time.h"

static void *time_func(void *x);

void start_time(void) {
    INFO("started time thread.");
    pthread_create(&time_th, NULL, time_func, NULL);
}

static void *time_func(void *x) {
    struct timespec absolute_time;
    int ret;

    pthread_mutex_init(&time_lock, NULL);
    while (!quit) {
        clock_gettime(CLOCK_REALTIME, &absolute_time);
        absolute_time.tv_sec += 30;
        ret = pthread_mutex_timedlock(&time_lock, &absolute_time);
        if (!ret || ret == ETIMEDOUT) {
            update_time();
        } else {
            WARN("an error occurred. Leaving.");
            pthread_mutex_unlock(&time_lock);
            break;
        }
    }
    pthread_mutex_destroy(&time_lock);
    pthread_exit(NULL);
}
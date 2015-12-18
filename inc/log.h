#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <pwd.h>
#include "declarations.h"

/*
 * Log levels
 */
#define LOG_ERR 0
#define LOG_WARN 1
#define LOG_INFO 2
#define NO_LOG 3

#define INFO(msg) log_message(__FILE__, __LINE__, __func__, msg, 'I', LOG_INFO)
#define WARN(msg) log_message(__FILE__, __LINE__, __func__, msg, 'W', LOG_WARN)
#define ERROR(msg) log_message(__FILE__, __LINE__, __func__, msg, 'E', LOG_ERR)

void open_log(void);
void log_current_options(void);
void log_message(const char *filename, int lineno, const char *funcname, const char *log_msg, char type, int log_level);
void close_log(void);
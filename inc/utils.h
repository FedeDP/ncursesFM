#pragma once

#include <magic.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"
#include "ui_functions.h"

void *remove_from_list(int *num, char (*str)[PATH_MAX + 1], int i);
void *safe_realloc(const size_t size, char (*str)[PATH_MAX + 1]);
int is_ext(const char *filename, const char *ext[], int size);
int get_mimetype(const char *path, const char *test);
int move_cursor_to_file(int start_idx, const char *filename, int win);
void save_old_pos(int win);
int is_present(const char *name, char (*str)[PATH_MAX + 1], int num, int len, int start_idx);
void change_unit(float size, char *str);
void leave_mode_helper(struct stat s);
void switch_back_normal_mode(int mode);

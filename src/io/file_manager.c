#include "file_manager.h"
#include "../editor/buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#  include <direct.h>   /* Windows: chdir, getcwd */
#else
#  include <unistd.h>
#endif

FileEntry file_list[MAX_FILE_COUNT];
int  file_count    = 0;
int  file_cursor   = 0;
int  file_scroll   = 0;
int  fm_display_limit = 20;   /* varsayilan; renderer tarafindan guncellenir */
char current_filename[256] = "";

void fm_set_display_limit(int limit) {
    if (limit > 0) fm_display_limit = limit;
}

void fm_read_dir() {
    DIR *dir = opendir(".");
    struct dirent *entry;
    struct stat st;

    file_count = 0;

    if (dir == NULL) return;

    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILE_COUNT) {
        if (strcmp(entry->d_name, ".") == 0) continue;
        strncpy(file_list[file_count].name, entry->d_name, 255);
        file_list[file_count].name[255] = '\0';
        if (stat(entry->d_name, &st) == 0)
            file_list[file_count].is_folder = S_ISDIR(st.st_mode);
        else
            file_list[file_count].is_folder = 0;
        file_count++;
    }

    closedir(dir);
}

void fm_cursor_up() {
    if (file_cursor > 0) {
        file_cursor--;
        if (file_cursor < file_scroll)
            file_scroll--;
    } else {
        file_cursor = file_count - 1;
        file_scroll = file_count - fm_display_limit;
        if (file_scroll < 0) file_scroll = 0;
    }
}

void fm_cursor_down() {
    if (file_cursor < file_count - 1) {
        file_cursor++;
        if (file_cursor >= file_scroll + fm_display_limit)
            file_scroll++;
    } else {
        file_cursor = 0;
        file_scroll = 0;
    }
}

void fm_clear_editor() {
    line_x *cur = headline;
    while (cur != NULL) {
        node_x *n = cur->dummynode;
        while (n != NULL) {
            node_x *nn = n->next;
            free(n);
            n = nn;
        }
        line_x *next = cur->nextline;
        free(cur);
        cur = next;
    }
    headline    = createnewline();
    currentline = headline;
    cursor      = currentline->dummynode;
}

void fm_load_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) return;

    strncpy(current_filename, filename, 255);
    current_filename[255] = '\0';

    fm_clear_editor();

    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '\r') continue;
        else if (c == '\n') addnewline();
        else letterEntry((char)c);
    }

    fclose(fp);

    cursor      = headline->dummynode;
    currentline = headline;
}

void fm_save_file(const char *filename) {
    if (filename == NULL || filename[0] == '\0') return;

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) return;

    line_x *line = headline;
    while (line != NULL) {
        node_x *node = line->dummynode->next;
        while (node != NULL) {
            fputc(node->letter, fp);
            node = node->next;
        }
        if (line->nextline != NULL)
            fputc('\n', fp);
        line = line->nextline;
    }

    fclose(fp);
}
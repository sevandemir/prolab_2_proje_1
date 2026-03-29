#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#define MAX_FILE_COUNT     512
#define BROWSER_DISPLAY_LIMIT 9999  /* renderer tarafindan dinamik olarak guncellenir */

typedef struct {
    char name[256];
    int  is_folder;
} FileEntry;

extern FileEntry file_list[MAX_FILE_COUNT];
extern int file_count;
extern int file_cursor;
extern int file_scroll;
extern int fm_display_limit;   /* renderer tarafindan set edilir */
extern char current_filename[256];

void fm_set_display_limit(int limit);

void fm_read_dir();
void fm_cursor_up();
void fm_cursor_down();
void fm_load_file(const char *filename);
void fm_save_file(const char *filename);
void fm_clear_editor();

#endif
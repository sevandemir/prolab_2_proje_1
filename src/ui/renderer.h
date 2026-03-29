#ifndef RENDERER_H
#define RENDERER_H

#include "../editor/buffer.h"
#include "../terminal/terminal.h"
#include "../editor/cursor.h"

extern int  save_active;
extern char save_filename_buf[256];
extern int browser_mode;

void renderer_draw(int terminal_rows, int terminal_cols);
void renderer_draw_filebrowser(int terminal_rows, int terminal_cols);
#endif // RENDERER_H
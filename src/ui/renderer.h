#ifndef RENDERER_H
#define RENDERER_H

#include "../editor/buffer.h"
#include "../terminal/terminal.h"
#include "../editor/cursor.h"

void renderer_draw(int terminal_rows, int terminal_cols);

#endif // RENDERER_H
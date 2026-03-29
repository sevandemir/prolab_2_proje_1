#ifndef TOOLBOX_H
#define TOOLBOX_H

#include "../terminal/terminal.h"

// Toolbox'ta gösterilecek menü öğesi
typedef struct {
    const char *label;      // Görünen isim: "Dosya Aç"
    const char *shortcut;   // Kısayol: "CTRL+O"
} ToolboxItem;

/* Doğrudan stdout'a yazar (eski arayüz – artık kullanılmıyor) */
void toolbox_render(int terminal_cols);

/* Renderer'ın ortak buffer'ına yazar – tek seferde flush için */
void toolbox_render_to_buf(char *buf, int *pos, int buf_size, int terminal_cols);

int  toolbox_row_count(int terminal_cols);

#endif // TOOLBOX_H
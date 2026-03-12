#ifndef TOOLBOX_H
#define TOOLBOX_H

#include "../terminal/terminal.h"

// Toolbox'ta gösterilecek menü öğesi
typedef struct {
    const char *label;      // Görünen isim: "Dosya Aç"
    const char *shortcut;   // Kısayol: "CTRL+O"
} ToolboxItem;

void toolbox_render(int terminal_cols);

#endif // TOOLBOX_H
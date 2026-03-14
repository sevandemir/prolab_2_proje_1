#include "toolbox.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static ToolboxItem items[] = {
    {"Dosya Ac",      "CTRL+O"},
    {"Kaydet",        "CTRL+S"},
    {"Farkli Kaydet", "CTRL+SHIFT+S"},
    {"Ara",           "CTRL+F"},
    {"Git",           "CTRL+G"},
    {"Degistir",      "CTRL+H"},
    {"Geri Al",       "CTRL+Z"},
    {"Kes",           "CTRL+X"},
    {"Kopyala",       "CTRL+C"},
    {"Yapistir",      "CTRL+V"},
    {"Cikis",         "ESC"},
};

static int item_count = sizeof(items) / sizeof(items[0]);

static void tb_write(const char *s) {
    write(STDOUT_FILENO, s, strlen(s));
}

void toolbox_render(int terminal_cols) {
    int x = 0;
    char buf[64];

    // Üst çizgi
    for (int i = 0; i < terminal_cols; i++) tb_write("-");
    tb_write("\r\n");

    // Öğeleri yan yana sığdır
    for (int i = 0; i < item_count; i++) {
        snprintf(buf, sizeof(buf), " %s(%s) ", items[i].label, items[i].shortcut);
        int len = strlen(buf);

        if (x + len > terminal_cols) {
            tb_write("\r\n");
            x = 0;
        }

        tb_write(buf);
        x += len;
    }

    tb_write("\r\n");

    // Alt çizgi
    for (int i = 0; i < terminal_cols; i++) tb_write("-");
    tb_write("\r\n");
}

int toolbox_row_count(int terminal_cols) {
    int rows = 2;
    int x = 0;
    for (int i = 0; i < item_count; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), " %s(%s) ", items[i].label, items[i].shortcut);
        int len = strlen(buf);
        if (x + len > terminal_cols) {
            rows++;
            x = 0;
        }
        x += len;
    }
    return rows;
}
#include "toolbox.h"
#include <stdio.h>
#include <string.h>

// Tüm komutlar burada tanımlı
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

void toolbox_render(int terminal_cols) {
    int x = 0;

    // Üst çizgi
    for (int i = 0; i < terminal_cols; i++) printf("-");
    printf("\r\n");

    // Öğeleri yan yana sığdır, sığmazsa alt satıra geç
    for (int i = 0; i < item_count; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), " %s(%s) ", items[i].label, items[i].shortcut);
        int len = strlen(buf);

        if (x + len > terminal_cols) {
            printf("\r\n");
            x = 0;
        }

        printf("%s", buf);
        x += len;
    }

    printf("\r\n");

    // Alt çizgi
    for (int i = 0; i < terminal_cols; i++) printf("-");
    printf("\r\n");
}
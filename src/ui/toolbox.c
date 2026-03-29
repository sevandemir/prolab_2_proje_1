#include "toolbox.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static ToolboxItem items[] = {
    {"Dosya Ac",      "CTRL+O"},
    {"Kaydet",        "CTRL+S"},
    {"Farkli Kaydet", "CTRL+W"},
    {"Ara",           "CTRL+F"},
    {"Git",           "CTRL+G"},
    {"Degistir",      "CTRL+R"},
    {"Geri Al",       "CTRL+Z"},
    {"Kes",           "CTRL+X"},
    {"Kopyala",       "CTRL+C"},
    {"Yapistir",      "CTRL+V"},
    {"Cikis",         "ESC"},
};

static int item_count = sizeof(items) / sizeof(items[0]);

/* ------------------------------------------------------------------ */
/* Yardımcı: buf'a güvenli string kopyalama                           */
/* ------------------------------------------------------------------ */
static void buf_write(char *buf, int *pos, int buf_size, const char *s) {
    int len = (int)strlen(s);
    if (*pos + len < buf_size) {
        memcpy(buf + *pos, s, len);
        *pos += len;
    }
}

static void buf_char(char *buf, int *pos, int buf_size, char c) {
    if (*pos < buf_size)
        buf[(*pos)++] = c;
}

/* ------------------------------------------------------------------ */
/* Renderer buffer'ına yazan yeni fonksiyon                            */
/* ------------------------------------------------------------------ */
void toolbox_render_to_buf(char *buf, int *pos, int buf_size, int terminal_cols) {
    /* Üst çizgi */
    for (int i = 0; i < terminal_cols; i++) buf_char(buf, pos, buf_size, '-');
    buf_write(buf, pos, buf_size, "\r\n");

    /* Öğeleri yan yana sığdır */
    int x = 0;
    char tmp[64];
    for (int i = 0; i < item_count; i++) {
        snprintf(tmp, sizeof(tmp), " %s(%s) ", items[i].label, items[i].shortcut);
        int len = (int)strlen(tmp);

        if (x + len > terminal_cols) {
            buf_write(buf, pos, buf_size, "\r\n");
            x = 0;
        }
        buf_write(buf, pos, buf_size, tmp);
        x += len;
    }
    buf_write(buf, pos, buf_size, "\r\n");

    /* Alt çizgi */
    for (int i = 0; i < terminal_cols; i++) buf_char(buf, pos, buf_size, '-');
    buf_write(buf, pos, buf_size, "\r\n");
}

/* ------------------------------------------------------------------ */
/* Geriye dönük uyumluluk – doğrudan stdout'a yaz (artık çağrılmıyor) */
/* ------------------------------------------------------------------ */
void toolbox_render(int terminal_cols) {
    char buf[4096];
    int  pos = 0;
    toolbox_render_to_buf(buf, &pos, (int)sizeof(buf), terminal_cols);
    write(STDOUT_FILENO, buf, pos);
}

/* ------------------------------------------------------------------ */
/* Kaç satır kapladığını hesapla                                       */
/* ------------------------------------------------------------------ */
int toolbox_row_count(int terminal_cols) {
    int rows = 2;  /* üst çizgi + alt çizgi */
    int x    = 0;
    for (int i = 0; i < item_count; i++) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), " %s(%s) ", items[i].label, items[i].shortcut);
        int len = (int)strlen(tmp);
        if (x + len > terminal_cols) {
            rows++;
            x = 0;
        }
        x += len;
    }
    return rows;  /* renderer.c'deki +1 formülü içerik satırını ekler */
}
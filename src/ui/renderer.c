#include "renderer.h"
#include "toolbox.h"
#include "../editor/selection.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static char render_buf[65536];
static int  render_pos = 0;

static void r_write(const char *s) {
    int len = strlen(s);
    if (render_pos + len < (int)sizeof(render_buf)) {
        memcpy(render_buf + render_pos, s, len);
        render_pos += len;
    }
}

static void r_char(char c) {
    if (render_pos < (int)sizeof(render_buf))
        render_buf[render_pos++] = c;
}

static void r_flush() {
    write(STDOUT_FILENO, render_buf, render_pos);
    render_pos = 0;
}

void renderer_draw(int terminal_rows, int terminal_cols) {
    render_pos = 0;
    r_write("\033[?25l");  // imleci gizle
    r_write("\033[H");
    r_flush();

    int toolbox_rows     = toolbox_row_count(terminal_cols);
    int max_lines        = terminal_rows - toolbox_rows - 3;
    int current_line_num = 1;

    line_x *tl = headline;
    while (tl != NULL && tl != currentline) {
        tl = tl->nextline;
        current_line_num++;
    }

    static int scroll_offset  = 0;
    static int h_scroll_offset = 0;

    if (current_line_num > scroll_offset + max_lines)
        scroll_offset = current_line_num - max_lines;
    if (current_line_num <= scroll_offset)
        scroll_offset = current_line_num - 1;

    // Yatay scroll: aktif satır için imleç pozisyonuna göre ayarla
    int text_cols = terminal_cols - 6;  // "NNN | " için 6 karakter ayrıldı
    int cur_pos   = cursorPosition();
    if (cur_pos >= h_scroll_offset + text_cols)
        h_scroll_offset = cur_pos - text_cols + 1;
    else if (cur_pos < h_scroll_offset + text_cols)
        h_scroll_offset = cur_pos > text_cols ? cur_pos - text_cols : 0;
    // 1. Toolbox
    toolbox_render(terminal_cols);
    fflush(stdout);

    // 2. Metin alanı
    line_x *templine = headline;
    int line_num = 1;
    while (templine != NULL && line_num <= scroll_offset) {
        templine = templine->nextline;
        line_num++;
    }

    int display_line = 1;
    int cursor_row   = toolbox_rows + 2;
    int cursor_col   = 7;

    char tmp[64];
    while (templine != NULL && display_line <= max_lines) {
        r_write("\033[2K");
        snprintf(tmp, sizeof(tmp), "%3d | ", line_num);
        r_write(tmp);

        // Aktif satırda yatay scroll uygula, diğer satırlarda başından göster
        int skip = (templine == currentline) ? h_scroll_offset : 0;
        int shown = 0;
        node_x *tempnode = templine->dummynode->next;

        // skip kadar node atla
        int skipped = 0;
        while (tempnode != NULL && skipped < skip) {
            tempnode = tempnode->next;
            skipped++;
        }

        while (tempnode != NULL && shown < text_cols) {
            if (selection_contains(templine, tempnode)) {
                r_write("\033[43m");
                r_char(tempnode->letter);
                r_write("\033[0m");
            } else {
                r_char(tempnode->letter);
            }
            tempnode = tempnode->next;
            shown++;
        }

        if (templine == currentline) {
            cursor_row = display_line + toolbox_rows + 1;
            cursor_col = 7 + (cur_pos - h_scroll_offset);
        }

        r_write("\r\n");
        templine = templine->nextline;
        line_num++;
        display_line++;
    }

    // Kalan satırları temizle
    for (int i = display_line; i <= max_lines; i++)
        r_write("\033[2K\r\n");

    // 3. Alt bilgi
    r_write("\033[2K");
    for (int i = 0; i < terminal_cols; i++) r_char('-');
    r_write("\r\n\033[2K");
    snprintf(tmp, sizeof(tmp), " Satir: %d  |  ESC: Cikis", current_line_num);
    r_write(tmp);

    // 4. İmleci konumlandır
    snprintf(tmp, sizeof(tmp), "\033[%d;%dH", cursor_row, cursor_col);
    r_write(tmp);

    r_flush();
    write(STDOUT_FILENO, "\033[?25h", 6);  // imleci göster
}
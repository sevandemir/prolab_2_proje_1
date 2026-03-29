#include "renderer.h"
#include "toolbox.h"
#include "../editor/selection.h"
#include "../editor/buffer.h"
#include "../search/search.h"
#include "../io/file_manager.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static char render_buf[65536];
static int  render_pos = 0;

int  save_active = 0;
char save_filename_buf[256] = {0};

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
    r_write("\033[?25l");
    r_write("\033[H");

    int toolbox_rows     = toolbox_row_count(terminal_cols);
    int max_lines        = terminal_rows - toolbox_rows - 3;
    int current_line_num = 1;

    line_x *tl = headline;
    while (tl != NULL && tl != currentline) {
        tl = tl->nextline;
        current_line_num++;
    }

    static int scroll_offset   = 0;
    static int h_scroll_offset = 0;

    if (current_line_num > scroll_offset + max_lines)
        scroll_offset = current_line_num - max_lines;
    if (current_line_num <= scroll_offset)
        scroll_offset = current_line_num - 1;

    int text_cols = terminal_cols - 6;
    int cur_pos   = cursorPosition();
    if (cur_pos >= h_scroll_offset + text_cols)
        h_scroll_offset = cur_pos - text_cols + 1;
    else if (cur_pos < h_scroll_offset + text_cols)
        h_scroll_offset = cur_pos > text_cols ? cur_pos - text_cols : 0;

    // 1. Toolbox – render buffer'a yaz (tek seferde flush olacak)
    toolbox_render_to_buf(render_buf, &render_pos, (int)sizeof(render_buf), terminal_cols);

    // 2. Metin alanı
    line_x *templine = headline;
    int line_num = 1;
    while (templine != NULL && line_num <= scroll_offset) {
        templine = templine->nextline;
        line_num++;
    }

    int display_line = 1;
    int cursor_row = toolbox_rows + 2;
    int cursor_col = 7;
    int line_idx = scroll_offset;

    char tmp[64];
    while (templine != NULL && display_line <= max_lines) {
        r_write("\033[2K");
        snprintf(tmp, sizeof(tmp), "%3d | ", line_num);
        r_write(tmp);

        int skip  = (templine == currentline) ? h_scroll_offset : 0;
        int shown = 0;
        node_x *tempnode = templine->dummynode->next;

        int skipped = 0;
        int col_idx = 0;
        while (tempnode != NULL && skipped < skip) {
            if (!IS_CONT_BYTE(tempnode->letter)) skipped++;
            tempnode = tempnode->next;
            col_idx++;
        }

        int current_bg = 0;
        while (tempnode != NULL && shown < text_cols) {
            int needed_bg = 0;
            if (selection_contains(templine, tempnode)) needed_bg = 1;
            else if (search_active && search_contains(line_idx, col_idx)) needed_bg = 2;

            if (needed_bg != current_bg) {
                if (needed_bg == 1) r_write("\033[43m");
                else if (needed_bg == 2) r_write("\033[44m");
                else r_write("\033[0m");
                current_bg = needed_bg;
            }

            r_char(tempnode->letter);

            if (!IS_CONT_BYTE(tempnode->letter)) shown++;
            tempnode = tempnode->next;
            col_idx++;
        }
        if (current_bg != 0) {
            r_write("\033[0m");
        }

        if (templine == currentline) {
            cursor_row = display_line + toolbox_rows + 1;
            cursor_col = 7 + (cur_pos - h_scroll_offset);
        }

        r_write("\r\n");
        templine = templine->nextline;
        line_num++;
        display_line++;
        line_idx++;
    }

    for (int i = display_line; i <= max_lines; i++)
        r_write("\033[2K\r\n");

    // 3. Alt bilgi
    r_write("\033[2K");
    for (int i = 0; i < terminal_cols; i++) r_char('-');
    r_write("\r\n\033[2K");

    if (save_active) {
        char stmp[320];
        snprintf(stmp, sizeof(stmp), " Dosya adi: %s", save_filename_buf);
        r_write(stmp);
    } else if (replace_active) {
        char stmp[320];
        snprintf(stmp, sizeof(stmp), " Yeni kelime: %s", replace_query);
        r_write(stmp);
    } else if (search_active) {
        char stmp[320];
        snprintf(stmp, sizeof(stmp), " Ara: %s", search_query);
        r_write(stmp);
    } else {
        snprintf(tmp, sizeof(tmp), " Satir: %d  |  ESC: Cikis", current_line_num);
        r_write(tmp);
    }

    // 4. İmleci konumlandır
    if (save_active) {
        snprintf(tmp, sizeof(tmp), "\033[%d;%dH", terminal_rows, (int)(13 + strlen(save_filename_buf)));
    } else {
        snprintf(tmp, sizeof(tmp), "\033[%d;%dH", cursor_row, cursor_col);
    }
    r_write(tmp);

    r_flush();
    write(STDOUT_FILENO, "\033[?25h", 6);
}


void renderer_draw_filebrowser(int terminal_rows, int terminal_cols) {
    render_pos = 0;
    r_write("\033[?25l");
    r_write("\033[H\033[2J");   /* imleç başa + tüm ekranı temizle */

    // Üst çizgi
    for (int i = 0; i < terminal_cols; i++) r_char('-');
    r_write("\r\n");

    // Başlık
    char tmp[256];
    snprintf(tmp, sizeof(tmp),
        " [DOSYA GEZGINI - %s]  |  YUK/AS: Gezin  |  ENTER: Sec  |  CTRL+S: Kaydet  |  ESC: Iptal\r\n",
        (browser_mode == 1) ? "AC" : "KAYDET"
    );
    r_write(tmp);

    for (int i = 0; i < terminal_cols; i++) r_char('-');
    r_write("\r\n");

    /* Başlık 3 satır + alt çizgi 1 + scroll bilgisi 1 + boş satır 1 = 6 satır rezerv */
    int display_limit = terminal_rows - 6;
    if (display_limit < 1) display_limit = 1;
    fm_set_display_limit(display_limit);  /* scroll sınırlarını senkronize et */

    int shown = 0;

    for (int i = file_scroll; i < file_count && shown < display_limit; i++) {
        char line[512];
        if (i == file_cursor) {
            if (file_list[i].is_folder)
                snprintf(line, sizeof(line), " -> [%s]/\r\n", file_list[i].name);
            else
                snprintf(line, sizeof(line), " -> %s\r\n", file_list[i].name);
        } else {
            if (file_list[i].is_folder)
                snprintf(line, sizeof(line), "    [%s]/\r\n", file_list[i].name);
            else
                snprintf(line, sizeof(line), "    %s\r\n", file_list[i].name);
        }
        r_write(line);
        shown++;
    }

    // Kalan satırları temizle
    for (int i = shown; i < display_limit; i++)
        r_write("\033[2K\r\n");

    // Alt çizgi
    for (int i = 0; i < terminal_cols; i++) r_char('-');
    r_write("\r\n");

    // Scroll göstergesi
    char scroll_info[128];
    int total_visible_end = file_scroll + display_limit;
    if (total_visible_end > file_count) total_visible_end = file_count;
    snprintf(scroll_info, sizeof(scroll_info),
        " Dosya: %d/%d  |  Gorunen: %d-%d\r\n",
        file_cursor + 1, file_count,
        file_count > 0 ? file_scroll + 1 : 0,
        total_visible_end
    );
    r_write(scroll_info);

    r_flush();
    write(STDOUT_FILENO, "\033[?25h", 6);
}


#include "renderer.h"
#include "toolbox.h"
#include <stdio.h>

void renderer_draw(int terminal_rows, int terminal_cols) {
    term_set_cursor(1, 1);

    // 1. Toolbox
    toolbox_render(terminal_cols);

    // 2. Metin alanı
    line_x *templine = headline;
    int line_num = 1;
    int max_lines = terminal_rows - 5;
    int cursor_row = 4;
    int cursor_col = 7;

    while (templine != NULL && line_num <= max_lines) {
        term_clear_line();
        printf("%3d | ", line_num);

        node_x *tempnode = templine->dummynode->next;
        while (tempnode != NULL) {
            printf("%c", tempnode->letter);
            tempnode = tempnode->next;
        }

        // İmleç konumunu hesapla
        if (templine == currentline) {
            cursor_row = line_num + 3; // toolbox 3 satır kaplar
            cursor_col = 7 + cursorPosition();
        }

        printf("\r\n");
        templine = templine->nextline;
        line_num++;
    }

    // 3. Alt bilgi
    term_clear_line();
    for (int i = 0; i < terminal_cols; i++) printf("-");
    printf("\r\n");
    term_clear_line();
    printf(" Satir: %d  |  ESC: Cikis\r\n", line_num - 1);

    // 4. İmleci doğru konuma taşı
    term_set_cursor(cursor_row, cursor_col);
}
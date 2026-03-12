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

    while (templine != NULL && line_num <= max_lines) {
        term_clear_line();
        printf("%3d | ", line_num);

        if (templine == currentline && cursor == templine->dummynode) {
            printf("\033[7m \033[0m");
        }

        node_x *tempnode = templine->dummynode->next;
        while (tempnode != NULL) {
            if (tempnode == cursor) {
                printf("\033[7m%c\033[0m", tempnode->letter);
            } else {
                printf("%c", tempnode->letter);
            }
            tempnode = tempnode->next;
        }

        if (templine == currentline && cursor != templine->dummynode) {
            node_x *last = templine->dummynode;
            while (last->next != NULL) last = last->next;
            if (cursor == last) {
                printf("\033[7m \033[0m");
            }
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
    // GEÇİCİ DEBUG
    printf(" Satir: %d  |  ESC: Cikis\r\n", line_num - 1);
}
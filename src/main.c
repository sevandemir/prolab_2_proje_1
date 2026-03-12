#include <stdio.h>
#include <unistd.h>
#include "terminal/terminal.h"
#include "editor/buffer.h"
#include "ui/renderer.h"
#include "undo/undo_stack.h"

int main(void) {
    term_enable_raw_mode();
    term_hide_cursor();
    term_cursor_blink_on();

    headline    = createnewline();
    currentline = headline;
    cursor      = currentline->dummynode;

    UndoStack *undo = undo_stack_create();

    int rows, cols;
    term_get_size(&rows, &cols);

    int running = 1;
    while (running) {
        renderer_draw(rows, cols);

        int key = term_read_key();

        if (key == 27) {
            running = 0;
            continue;
        }

        switch (key) {
            case KEY_ENTER:
                undo_push(undo, OP_INSERT_LINE, 0, 0, NULL);
                addnewline();
                break;
            case KEY_BACKSPACE:
                undo_push(undo, OP_DELETE_CHAR, 0, 0, NULL);
                deleteLetter();
                break;
            case KEY_UP:    cursorToUp();    break;
            case KEY_DOWN:  cursorToDown();  break;
            case KEY_LEFT:  cursorToLeft();  break;
            case KEY_RIGHT: cursorToRight(); break;
            default:
                if (key >= 32 && key <= 126) {
                    undo_push(undo, OP_INSERT_CHAR, 0, 0, NULL);
                    letterEntry((char)key);
                }
                break;
        }
    }

    // Temizlik - döngü dışında
    undo_stack_destroy(undo);
    term_cursor_blink_off();
    term_reset_color();
    term_show_cursor();
    term_disable_raw_mode();
    term_clear_screen();

    printf("Cikis yapildi.\n");
    return 0;
}

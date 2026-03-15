#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "terminal/terminal.h"
#include "editor/buffer.h"
#include "ui/renderer.h"
#include "undo/undo_stack.h"
#include "editor/cursor.h"
#include "editor/selection.h"
#include "editor/editor.h"
#include "editor/search.h"

static void read_input(char *buf, int max_len, int terminal_rows, int terminal_cols) {
    int pos = 0;
    buf[0] = '\0';
    while (1) {
        renderer_draw(terminal_rows, terminal_cols);
        int key = term_read_key();
        if (key == KEY_ESC) {
            buf[0] = '\0';
            return;
        }
        if (key == KEY_ENTER) return;
        if (key == KEY_BACKSPACE) {
            if (pos > 0) buf[--pos] = '\0';
        } else if (key >= 32 && key <= 126 && pos < max_len - 1) {
            buf[pos++] = (char)key;
            buf[pos]   = '\0';
        }
    }
}

int main(void) {
    term_enable_raw_mode();
    write(STDOUT_FILENO, "\033[?1049h", 8);

    term_cursor_blink_on();

    headline    = createnewline();
    currentline = headline;
    cursor      = currentline->dummynode;

    undo = undo_stack_create();

    int rows, cols;
    term_get_size(&rows, &cols);

    int running = 1;
    while (running) {
        renderer_draw(rows, cols);

        int key = term_read_key();

        if (key == KEY_ESC) {
            if (search_active || replace_active) {
                search_clear();
            } else {
                running = 0;
            }
            continue;
        }

        switch (key) {
            case KEY_ENTER:
                if (selection_active) deleteSelection();
                addnewline();
                break;
            case KEY_BACKSPACE:
                if (selection_active) deleteSelection();
                else deleteLetter();
                break;
            case KEY_UP:    selection_clear(); cursorToUp();    break;
            case KEY_DOWN:  selection_clear(); cursorToDown();  break;
            case KEY_LEFT:  selection_clear(); cursorToLeft();  break;
            case KEY_RIGHT: selection_clear(); cursorToRight(); break;
            case KEY_SHIFT_LEFT:  selection_shift_left();  break;
            case KEY_SHIFT_RIGHT: selection_shift_right(); break;
            case KEY_SHIFT_UP:    selection_shift_up();    break;
            case KEY_SHIFT_DOWN:  selection_shift_down();  break;
            case KEY_CTRL_C: copySelection();  break;
            case KEY_CTRL_X: cutSelection();   break;
            case KEY_CTRL_V: pasteClipboard(); break;
            case KEY_CTRL_Z: performUndo();    break;
            case KEY_CTRL_F: {
                search_active = 1;
                read_input(search_query, sizeof(search_query), rows, cols);
                if (search_query[0] != '\0') {
                    search_find(search_query);
                    if (search_matches != NULL) search_next();
                } else {
                    search_clear();
                }
                break;
            }
            case KEY_CTRL_G: {
                if (search_active && search_matches != NULL) {
                    search_next();
                }
                break;
            }
            case KEY_CTRL_H: {
                search_active  = 1;
                replace_active = 0;
                read_input(search_query, sizeof(search_query), rows, cols);
                if (search_query[0] == '\0') { search_clear(); break; }
                search_active  = 0;
                replace_active = 1;
                read_input(replace_query, sizeof(replace_query), rows, cols);
                if (replace_query[0] != '\0') {
                    search_replace_all(search_query, replace_query);
                }
                search_clear();
                break;
            }
            default:
                if (key >= 32 && key <= 126)
                    letterEntry((char)key);
                break;
        }
    }

    undo_stack_destroy(undo);
    term_cursor_blink_off();
    term_reset_color();
    term_show_cursor();
    term_disable_raw_mode();
    write(STDOUT_FILENO, "\033[?1049l", 8);
    write(STDOUT_FILENO, "Cikis yapildi.\n", 15);
    return 0;
}
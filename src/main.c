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
#include "io/file_manager.h"
#include "search/search.h"

int browser_mode;

static void read_input(char *buf, int max_len, int terminal_rows, int terminal_cols) {
    int pos = strlen(buf);
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
        } else if (key >= 32 && key <= 255 && key != 127 && pos < max_len - 1) {
            /* 128-255 arası byte'lar UTF-8 / Türkçe karakterlerin parçalarıdır */
            buf[pos++] = (char) key;
            buf[pos] = '\0';
        }
    }
}

int main(void) {
    term_enable_raw_mode();
    write(STDOUT_FILENO, "\033[?1049h", 8);

    term_cursor_blink_on();

    headline = createnewline();
    currentline = headline;
    cursor = currentline->dummynode;

    undo = undo_stack_create();

    int rows, cols;
    term_get_size(&rows, &cols);


    int running = 1;
    /* Bracketed paste takibi */
    int paste_in_progress = 0;
    int paste_start_line  = 0;
    int paste_start_col   = 0;
    /* CTRL+G eşleşmeler arası gezinme modu */
    int navigate_mode = 0;

    while (running) {
        if (browser_mode == 1 || browser_mode == 2) {
            renderer_draw_filebrowser(rows, cols);

            // Kaydet modunda alt bilgiyi güncelle
            if (browser_mode == 2) {
                char prompt[512];
                snprintf(prompt, sizeof(prompt),
                         "\033[%d;1H\033[2K Gezin ve CTRL+S ile kaydet | Dosya adi: %s",
                         rows, save_filename_buf);
                write(STDOUT_FILENO, prompt, strlen(prompt));
            }

            int key = term_read_key();

            if (key == KEY_ESC) {
                browser_mode = 0;
                save_filename_buf[0] = '\0';
            } else if (key == KEY_UP) {
                fm_cursor_up();
            } else if (key == KEY_DOWN) {
                fm_cursor_down();
            } else if (key == KEY_ENTER) {
                if (file_list[file_cursor].is_folder) {
                    if (chdir(file_list[file_cursor].name) == 0) {
                        file_cursor = 0;
                        file_scroll = 0;
                        fm_read_dir();
                    }
                } else if (browser_mode == 1) {
                    fm_load_file(file_list[file_cursor].name);
                    browser_mode = 0;
                }
            } else if (key == KEY_CTRL_S && browser_mode == 2) {
                // Mevcut dizine kaydet
                if (save_filename_buf[0] != '\0') {
                    strncpy(current_filename, save_filename_buf, sizeof(current_filename));
                    fm_save_file(current_filename);
                    save_filename_buf[0] = '\0';
                    browser_mode = 0;
                }
            }
            continue;
        }

        renderer_draw(rows, cols);
        int key = term_read_key();

        if (key == KEY_ESC) {
            navigate_mode = 0;
            if (search_active || replace_active) {
                search_clear();
            } else {
                running = 0;
            }
            continue;
        }

        switch (key) {
            case KEY_ENTER:
                if (search_active || replace_active) { search_clear(); navigate_mode = 0; }
                if (selection_active) deleteSelection();
                addnewline();
                break;
            case KEY_BACKSPACE:
                if (search_active || replace_active) { search_clear(); navigate_mode = 0; }
                if (selection_active) deleteSelection();
                else deleteLetter();
                break;
            case KEY_UP:
                if (navigate_mode && search_matches != NULL) {
                    search_prev();
                } else {
                    selection_clear();
                    cursorToUp();
                }
                break;
            case KEY_DOWN:
                if (navigate_mode && search_matches != NULL) {
                    search_next();
                } else {
                    selection_clear();
                    cursorToDown();
                }
                break;
            case KEY_LEFT:
                if (navigate_mode && search_matches != NULL) {
                    search_prev();
                } else {
                    selection_clear();
                    cursorToLeft();
                }
                break;
            case KEY_RIGHT:
                if (navigate_mode && search_matches != NULL) {
                    search_next();
                } else {
                    selection_clear();
                    cursorToRight();
                }
                break;
            case KEY_SHIFT_LEFT: selection_shift_left();
                break;
            case KEY_SHIFT_RIGHT: selection_shift_right();
                break;
            case KEY_SHIFT_UP: selection_shift_up();
                break;
            case KEY_SHIFT_DOWN: selection_shift_down();
                break;
            case KEY_CTRL_C: copySelection();
                break;
            case KEY_CTRL_X: cutSelection();
                break;
            case KEY_CTRL_V: pasteClipboard();
                break;
            case KEY_CTRL_Z: performUndo();
                break;
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
                /* CTRL+F ile aynı: input al, ara, ilk eşleşmeye git */
                /* Sonra yön tuşlarıyla eşleşmeler arası gezinme modu açılır */
                search_active = 1;
                navigate_mode = 0;
                read_input(search_query, sizeof(search_query), rows, cols);
                if (search_query[0] != '\0') {
                    search_find(search_query);
                    if (search_matches != NULL) {
                        search_next();
                        navigate_mode = 1;  /* gezinme modunu aç */
                    }
                } else {
                    search_clear();
                }
                break;
            }
            case KEY_CTRL_R: {
                search_active = 1;
                replace_active = 0;
                read_input(search_query, sizeof(search_query), rows, cols);
                if (search_query[0] == '\0') {
                    search_clear();
                    break;
                }
                search_active = 0;
                replace_active = 1;
                read_input(replace_query, sizeof(replace_query), rows, cols);
                if (replace_query[0] != '\0')
                    search_replace_all(search_query, replace_query);
                search_clear();
                break;
            }
            case KEY_CTRL_O: {
                file_cursor = 0;
                file_scroll = 0;
                fm_read_dir();
                browser_mode = 1;
                break;
            }
            case KEY_CTRL_S: {
                if (current_filename[0] != '\0') {
                    // Zaten dosya adı var, direkt kaydet
                    fm_save_file(current_filename);
                } else {
                    // Önce dosya adı al
                    save_active = 1;
                    save_filename_buf[0] = '\0';
                    read_input(save_filename_buf, sizeof(save_filename_buf), rows, cols);
                    save_active = 0;
                    if (save_filename_buf[0] != '\0') {
                        // Dosya gezginini aç
                        file_cursor = 0;
                        file_scroll = 0;
                        fm_read_dir();
                        browser_mode = 2;
                    }
                }
                break;
            }
            case KEY_CTRL_W: {
                // Her zaman Save As

                save_active = 1;
                save_filename_buf[0] = '\0';

                // Dosya adını kullanıcıdan al
                read_input(save_filename_buf, sizeof(save_filename_buf), rows, cols);

                save_active = 0;

                if (save_filename_buf[0] != '\0') {
                    // Explorer aç
                    file_cursor = 0;
                    file_scroll = 0;
                    fm_read_dir();
                    browser_mode = 2;
                }

                break;
            }
            case KEY_PASTE_START: {
                /* Terminal yapıştırma başladı — tüm karakterleri tek undo bloğuna topla */
                paste_in_progress = 1;
                undo_enabled = 0;
                /* Başlangıç konumunu kaydet */
                paste_start_line = 0;
                line_x *ptl = headline;
                while (ptl && ptl != currentline) { paste_start_line++; ptl = ptl->nextline; }
                paste_start_col = 0;
                node_x *pnd = currentline->dummynode;
                while (pnd != cursor) { paste_start_col++; pnd = pnd->next; }
                break;
            }
            case KEY_PASTE_END: {
                /* Terminal yapıştırma bitti — tek undo kaydı ekle */
                undo_enabled = 1;
                if (paste_in_progress) {
                    paste_in_progress = 0;
                    int el = 0;
                    line_x *ptl = headline;
                    while (ptl && ptl != currentline) { el++; ptl = ptl->nextline; }
                    int ec = 0;
                    node_x *pnd = currentline->dummynode;
                    while (pnd != cursor) { ec++; pnd = pnd->next; }
                    char enc[128];
                    snprintf(enc, sizeof(enc), "PASTE:%d:%d:%d:%d",
                             paste_start_line, paste_start_col, el, ec);
                    undo_push(undo, OP_REPLACE, 0, 0, enc);
                }
                break;
            }
            default:
                /* 128-255 arası byte'lar UTF-8 / Türkçe karakterlerin parçalarıdır */
                if (key >= 32 && key <= 255 && key != 127) {
                    if (search_active || replace_active) { search_clear(); navigate_mode = 0; }
                    letterEntry((char) key);
                }
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

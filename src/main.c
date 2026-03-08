#include "terminal/terminal.h"
#include "editor/buffer.h"
#include "editor/editor.h"
#include "editor/cursor.h"
#include "ui/toolbox.h"
#include "ui/renderer.h"
#include "undo/undo_stack.h"

int main(void) {
    // Terminal'i raw moda al
    term_enable_raw_mode();
    term_cursor_blink_on();
    term_clear_screen();

    // TODO: buffer, cursor, undo_stack başlat
    // TODO: editor döngüsünü başlat

    // Çıkışta terminal'i eski haline getir
    term_cursor_blink_off();
    term_reset_color();
    term_show_cursor();
    term_disable_raw_mode();
    term_clear_screen();

    return 0;
}
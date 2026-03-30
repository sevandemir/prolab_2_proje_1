#include "selection.h"

#include <stddef.h>

#include "../editor/cursor.h"

int selection_active = 0;
SelectionPoint sel_start = {NULL, NULL};
SelectionPoint sel_end   = {NULL, NULL};

void selection_start() {
    if (!selection_active) {
        sel_start.line = currentline;
        sel_start.node = cursor;  // imlecin solundaki node
        sel_end.line   = currentline;
        sel_end.node   = cursor;
        selection_active = 1;
    }
}

void selection_clear() {
    selection_active = 0;
    sel_start.line = NULL;
    sel_start.node = NULL;
    sel_end.line   = NULL;
    sel_end.node   = NULL;
}

// Bir node'un seçim içinde olup olmadığını kontrol et
int selection_contains(line_x *line, node_x *node) {
    if (!selection_active) return 0;

    // Başlangıç ve bitiş sıralaması için satır + kolon pozisyonu bul
    // Önce start ve end'in sırasını belirle
    line_x *a_line = sel_start.line;
    node_x *a_node = sel_start.node;
    line_x *b_line = sel_end.line;
    node_x *b_node = sel_end.node;

    // a başta b sonda olsun diye sırala
    // Satır sırasını bul
    int a_before_b = 0;
    if (a_line == b_line) {
        // Aynı satırda, node sırasına bak
        node_x *tmp = a_node;
        while (tmp != NULL) {
            if (tmp == b_node) { a_before_b = 1; break; }
            tmp = tmp->next;
        }
    } else {
        line_x *tmp = a_line;
        while (tmp != NULL) {
            if (tmp == b_line) { a_before_b = 1; break; }
            tmp = tmp->nextline;
        }
    }

    if (!a_before_b) {
        // swap
        line_x *tl = a_line; a_line = b_line; b_line = tl;
        node_x *tn = a_node; a_node = b_node; b_node = tn;
    }

    // Şimdi a_line/a_node başlangıç, b_line/b_node bitiş
    // line, bu aralıkta mı?
    int found_start = 0;
    line_x *tmp = a_line;
    while (tmp != NULL) {
        if (tmp == line) { found_start = 1; break; }
        if (tmp == b_line) break;
        tmp = tmp->nextline;
    }
    if (!found_start) return 0;

    // Aynı satırdaysa node kontrolü
    if (a_line == b_line && line == a_line) {
        node_x *tn = a_node->next;
        while (tn != NULL && tn != b_node->next) {
            if (tn == node) return 1;
            tn = tn->next;
        }
        return 0;
    }

    if (line == a_line) {
        node_x *tn = a_node->next;
        while (tn != NULL) {
            if (tn == node) return 1;
            tn = tn->next;
        }
        return 0;
    }

    if (line == b_line) {
        node_x *tn = b_line->dummynode->next;
        while (tn != NULL && tn != b_node->next) {
            if (tn == node) return 1;
            tn = tn->next;
        }
        return 0;
    }

    // Ortadaki satırlar — tamamı seçili
    return 1;
}

void selection_shift_left() {
    selection_start();
    cursorToLeft();
    sel_end.line = currentline;
    sel_end.node = cursor;
}

void selection_shift_right() {
    selection_start();
    cursorToRight();
    sel_end.line = currentline;
    sel_end.node = cursor;  // cursor sağa gitti, şimdi seçilen karakteri gösteriyor
}

void selection_shift_up() {
    selection_start();
    cursorToUp();
    sel_end.line = currentline;
    sel_end.node = cursor;
}

void selection_shift_down() {
    selection_start();
    cursorToDown();
    sel_end.line = currentline;
    sel_end.node = cursor;
}

void selection_word_right() {
    selection_start();
    cursor_skip_word_right();
    sel_end.line = currentline;
    sel_end.node = cursor;
}

void selection_word_left() {
    selection_start();
    cursor_skip_word_left();
    sel_end.line = currentline;
    sel_end.node = cursor;
}
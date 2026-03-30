#include "cursor.h"
#include <ctype.h>


#include <stddef.h>

#include "buffer.h"

int cursorPosition() {
    int vis = 0;
    node_x *temp = currentline->dummynode;
    while (temp != cursor && temp->next != NULL) {
        temp = temp->next;
        if (!IS_CONT_BYTE(temp->letter)) vis++;
    }
    return vis;
}

void cursorToLeft() {
    if (cursor == currentline->dummynode) return;
    cursor = cursor->prev;
    while (cursor != currentline->dummynode && cursor->next != NULL && IS_CONT_BYTE(cursor->next->letter)) {
        cursor = cursor->prev;
    }
}

void cursorToRight() {
    if (cursor->next == NULL) return;
    cursor = cursor->next;
    while (cursor->next != NULL && IS_CONT_BYTE(cursor->next->letter)) {
        cursor = cursor->next;
    }
}

void cursorToUp() {
    int vis_pos = cursorPosition();
    if (currentline->prevline == NULL) return;
    currentline = currentline->prevline;
    cursor = currentline->dummynode;

    int current_vis = 0;
    while (cursor->next != NULL) {
        if (!IS_CONT_BYTE(cursor->next->letter)) {
            if (current_vis == vis_pos) break;
            current_vis++;
        }
        cursor = cursor->next;
    }
}

void cursor_skip_word_right() {
    if (cursor->next == NULL && currentline->nextline != NULL) {
        currentline = currentline->nextline;
        cursor = currentline->dummynode;
        return;
    }
    if (cursor->next == NULL) return;

    if (isspace((unsigned char)cursor->next->letter)) {
        while (cursor->next != NULL && isspace((unsigned char)cursor->next->letter)) {
            cursorToRight();
        }
    } else {
        while (cursor->next != NULL && !isspace((unsigned char)cursor->next->letter)) {
            cursorToRight();
        }
    }
}

void cursor_skip_word_left() {
    if (cursor == currentline->dummynode && currentline->prevline != NULL) {
        currentline = currentline->prevline;
        cursor = currentline->dummynode;
        while (cursor->next != NULL) {
            cursor = cursor->next;
        }
        return;
    }
    if (cursor == currentline->dummynode) return;

    if (isspace((unsigned char)cursor->letter)) {
        while (cursor != currentline->dummynode && isspace((unsigned char)cursor->letter)) {
            cursorToLeft();
        }
    } else {
        while (cursor != currentline->dummynode && !isspace((unsigned char)cursor->letter)) {
            cursorToLeft();
        }
    }
}

void cursorToDown() {
    int vis_pos = cursorPosition();
    if (currentline->nextline == NULL) return;
    currentline = currentline->nextline;
    cursor = currentline->dummynode;

    int current_vis = 0;
    while (cursor->next != NULL) {
        if (!IS_CONT_BYTE(cursor->next->letter)) {
            if (current_vis == vis_pos) break;
            current_vis++;
        }
        cursor = cursor->next;
    }
}
#include "cursor.h"

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
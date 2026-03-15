#include "cursor.h"

#include <stddef.h>

#include "buffer.h"

int cursorPosition() {
    int count = 0;
    node_x *temp = currentline->dummynode;
    while (temp != cursor) {
        temp = temp->next;
        count++;
    }
    return count;
}

void cursorToLeft() {
    if (cursor != currentline->dummynode)
        cursor = cursor->prev;
}

void cursorToRight() {
    if (cursor->next != NULL)
        cursor = cursor->next;
}

void cursorToUp() {
    int pos = cursorPosition();
    if (currentline->prevline == NULL) return;
    currentline = currentline->prevline;
    cursor = currentline->dummynode;
    for (int i = 0; i < pos; i++) {
        if (cursor->next == NULL) break;
        cursor = cursor->next;
    }
}

void cursorToDown() {
    int pos = cursorPosition();
    if (currentline->nextline == NULL) return;
    currentline = currentline->nextline;
    cursor = currentline->dummynode;
    for (int i = 0; i < pos; i++) {
        if (cursor->next == NULL) break;
        cursor = cursor->next;
    }
}
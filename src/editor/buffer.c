#include <stdio.h>
#include <stdlib.h>
#include "buffer.h"

line_x *headline    = NULL;
node_x *cursor      = NULL;
line_x *currentline = NULL;

node_x *createnewnode(char letter) {
    node_x *newnode = malloc(sizeof(node_x));
    newnode->letter = letter;
    newnode->prev   = NULL;
    newnode->next   = NULL;
    return newnode;
}

line_x *createnewline() {
    line_x *newline = malloc(sizeof(line_x));
    newline->dummynode = createnewnode('\0');
    newline->nextline  = NULL;
    newline->prevline  = NULL;
    return newline;
}

void letterEntry(char entry) {
    node_x *newnode = createnewnode(entry);
    newnode->next = cursor->next;
    newnode->prev = cursor;
    if (cursor->next != NULL)
        cursor->next->prev = newnode;
    cursor->next = newnode;
    cursor = newnode;
}

void cursorToLeft() {
    if (cursor != currentline->dummynode)
        cursor = cursor->prev;
}

void cursorToRight() {
    if (cursor->next != NULL)
        cursor = cursor->next;
}

int cursorPosition() {
    int count = 0;
    node_x *temp = currentline->dummynode;
    while (temp != cursor) {
        temp = temp->next;
        count++;
    }
    return count;
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

void mergeLines() {
    if (currentline->prevline == NULL) return;

    line_x *deletedLine = currentline;
    line_x *upperLine   = currentline->prevline;
    node_x *upperLast   = upperLine->dummynode;

    while (upperLast->next != NULL)
        upperLast = upperLast->next;

    if (deletedLine->dummynode->next != NULL) {
        upperLast->next = deletedLine->dummynode->next;
        deletedLine->dummynode->next->prev = upperLast;
    }

    upperLine->nextline = deletedLine->nextline;
    if (deletedLine->nextline != NULL)
        deletedLine->nextline->prevline = upperLine;

    cursor      = upperLast;
    currentline = upperLine;

    free(deletedLine->dummynode);
    free(deletedLine);
}

void deleteLetter() {
    if (cursor == currentline->dummynode) {
        if (currentline->prevline != NULL) {
            mergeLines();
            return;
        }
        return;
    }
    node_x *deletednode  = cursor;
    node_x *targetcursor = cursor->prev;
    if (targetcursor == NULL) return;

    if (cursor->next != NULL) cursor->next->prev = cursor->prev;
    if (cursor->prev != NULL) cursor->prev->next = cursor->next;

    cursor = targetcursor;
    free(deletednode);
}

void addnewline() {
    line_x *newline = createnewline();

    newline->prevline = currentline;
    newline->nextline = currentline->nextline;
    if (currentline->nextline != NULL)
        currentline->nextline->prevline = newline;
    currentline->nextline = newline;

    if (cursor->next != NULL) {
        newline->dummynode->next   = cursor->next;
        cursor->next->prev         = newline->dummynode;
        cursor->next               = NULL;
    }

    currentline = newline;
    cursor      = currentline->dummynode;
}
#include <stdio.h>
#include <stdlib.h>
#include "buffer.h"
#include "../undo/undo_stack.h"

line_x *headline    = NULL;
node_x *cursor      = NULL;
line_x *currentline = NULL;
int     undo_enabled = 1;

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
    if (undo && undo_enabled) {
        char text[2] = {entry, '\0'};
        int col = 0;
        node_x *tmp = currentline->dummynode;
        while (tmp != cursor) { col++; tmp = tmp->next; }
        int line = 0;
        line_x *tl = headline;
        while (tl != NULL && tl != currentline) { line++; tl = tl->nextline; }
        undo_push(undo, OP_INSERT_CHAR, line, col, text);
    }

    node_x *newnode = createnewnode(entry);
    newnode->next = cursor->next;
    newnode->prev = cursor;
    if (cursor->next != NULL)
        cursor->next->prev = newnode;
    cursor->next = newnode;
    cursor = newnode;
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
            if (undo && undo_enabled) {
                int line = 0;
                line_x *tl = headline;
                while (tl != NULL && tl != currentline) { line++; tl = tl->nextline; }
                undo_push(undo, OP_DELETE_LINE, line, 0, NULL);
            }
            mergeLines();
            return;
        }
        return;
    }

    if (undo && undo_enabled) {
        char text[2] = {cursor->letter, '\0'};
        int col = 0;
        node_x *tmp = currentline->dummynode;
        while (tmp != cursor) { col++; tmp = tmp->next; }
        int line = 0;
        line_x *tl = headline;
        while (tl != NULL && tl != currentline) { line++; tl = tl->nextline; }
        undo_push(undo, OP_DELETE_CHAR, line, col, text);
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
    if (undo && undo_enabled) {
        int line = 0;
        line_x *tl = headline;
        while (tl != NULL && tl != currentline) { line++; tl = tl->nextline; }
        undo_push(undo, OP_INSERT_LINE, line, 0, NULL);
    }

    line_x *newline = createnewline();

    newline->prevline = currentline;
    newline->nextline = currentline->nextline;
    if (currentline->nextline != NULL)
        currentline->nextline->prevline = newline;
    currentline->nextline = newline;

    if (cursor->next != NULL) {
        newline->dummynode->next = cursor->next;
        cursor->next->prev       = newline->dummynode;
        cursor->next             = NULL;
    }

    currentline = newline;
    cursor      = currentline->dummynode;
}
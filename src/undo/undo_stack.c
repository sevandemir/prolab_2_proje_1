#include <stdlib.h>
#include <string.h>
#include "undo_stack.h"

UndoStack *undo_stack_create() {
    UndoStack *stack = malloc(sizeof(UndoStack));
    stack->top  = NULL;
    stack->size = 0;
    return stack;
}

void undo_push(UndoStack *stack, OpType type, int line, int col, const char *text) {
    UndoRecord *rec = malloc(sizeof(UndoRecord));
    rec->type       = type;
    rec->line_index = line;
    rec->col        = col;
    rec->text       = text ? strdup(text) : NULL;
    rec->next       = stack->top;
    stack->top      = rec;
    stack->size++;
}

UndoRecord *undo_pop(UndoStack *stack) {
    if (stack->top == NULL) return NULL;
    UndoRecord *rec = stack->top;
    stack->top      = rec->next;
    stack->size--;
    return rec;   // çağıran undo_record_destroy ile free etmeli
}

void undo_record_destroy(UndoRecord *rec) {
    if (rec == NULL) return;
    free(rec->text);
    free(rec);
}

void undo_stack_destroy(UndoStack *stack) {
    UndoRecord *cur = stack->top;
    while (cur != NULL) {
        UndoRecord *next = cur->next;
        undo_record_destroy(cur);
        cur = next;
    }
    free(stack);
}
#ifndef UNDO_STACK_H
#define UNDO_STACK_H

typedef enum {
    OP_INSERT_CHAR,
    OP_DELETE_CHAR,
    OP_INSERT_LINE,
    OP_DELETE_LINE,
    OP_REPLACE
} OpType;

typedef struct UndoRecord {
    OpType  type;
    int     line_index;
    int     col;
    int     end_line_index;  // ← yeni
    int     end_col;         // ← yeni
    char   *text;
    struct UndoRecord *next;
} UndoRecord;

typedef struct {
    UndoRecord *top;
    int         size;
} UndoStack;

extern UndoStack *undo;

UndoStack  *undo_stack_create();
void        undo_stack_destroy(UndoStack *stack);
void        undo_push(UndoStack *stack, OpType type, int line, int col, const char *text);
UndoRecord *undo_pop(UndoStack *stack);
void        undo_record_destroy(UndoRecord *rec);

#endif
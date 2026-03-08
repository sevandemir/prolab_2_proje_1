#ifndef UNDO_STACK_H
#define UNDO_STACK_H

// ─── İşlem türleri ────────────────────────────────────────────────────────────
typedef enum {
    OP_INSERT_CHAR,
    OP_DELETE_CHAR,
    OP_INSERT_LINE,
    OP_DELETE_LINE,
    OP_REPLACE
} OpType;

// ─── Tek bir undo kaydı ───────────────────────────────────────────────────────
typedef struct UndoRecord {
    OpType  type;
    int     line_index;
    int     col;
    char   *text;           // Silinen/eklenen metin (kopyası)
    struct UndoRecord *next;
} UndoRecord;

// ─── Stack ────────────────────────────────────────────────────────────────────
typedef struct {
    UndoRecord *top;
    int         size;
} UndoStack;

UndoStack  *undo_stack_create();
void        undo_stack_destroy(UndoStack *stack);
void        undo_push(UndoStack *stack, OpType type, int line, int col, const char *text);
UndoRecord *undo_pop(UndoStack *stack);
void        undo_record_destroy(UndoRecord *rec);

#endif // UNDO_STACK_H
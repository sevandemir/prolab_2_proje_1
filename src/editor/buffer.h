#ifndef BUFFER_H
#define BUFFER_H

typedef struct node {
    char        letter;
    struct node *prev;
    struct node *next;
} node_x;

typedef struct line {
    node_x      *dummynode;
    struct line *prevline;
    struct line *nextline;
} line_x;

extern line_x *headline;
extern node_x *cursor;
extern line_x *currentline;
extern int undo_enabled;

node_x *createnewnode(char letter);
line_x *createnewline(void);
void    letterEntry(char entry);
void    mergeLines(void);
void    deleteLetter(void);
void    addnewline(void);

#endif
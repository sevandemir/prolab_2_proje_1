#include "editor.h"
#include "buffer.h"
#include "selection.h"
#include "../undo/undo_stack.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct ClipLine {
    char *text;
    struct ClipLine *next;
} ClipLine;

static ClipLine *clipboard = NULL;

static void clipboard_clear() {
    ClipLine *cur = clipboard;
    while (cur != NULL) {
        ClipLine *next = cur->next;
        free(cur->text);
        free(cur);
        cur = next;
    }
    clipboard = NULL;
}

static int node_before(node_x *a, node_x *b) {
    node_x *tmp = a;
    while (tmp != NULL) {
        if (tmp == b) return 1;
        tmp = tmp->next;
    }
    return 0;
}

static int line_before(line_x *a, line_x *b) {
    line_x *tmp = a;
    while (tmp != NULL) {
        if (tmp == b) return 1;
        tmp = tmp->nextline;
    }
    return 0;
}

void deleteSelection() {
    if (!selection_active) return;

    line_x *a_line = sel_start.line;
    node_x *a_node = sel_start.node;
    line_x *b_line = sel_end.line;
    node_x *b_node = sel_end.node;

    int a_before_b = 0;
    if (a_line == b_line)
        a_before_b = node_before(a_node, b_node);
    else
        a_before_b = line_before(a_line, b_line);

    if (!a_before_b) {
        line_x *tl = a_line;
        a_line = b_line;
        b_line = tl;
        node_x *tn = a_node;
        a_node = b_node;
        b_node = tn;
    }

    currentline = a_line;
    cursor = a_node;

    if (a_line == b_line) {
        node_x *cur = a_node->next;
        while (cur != NULL && cur != b_node->next) {
            node_x *next = cur->next;
            a_node->next = next;
            if (next != NULL) next->prev = a_node;
            free(cur);
            cur = next;
        }
    } else {
        node_x *b_rest = b_node->next;

        node_x *cur = a_node->next;
        while (cur != NULL) {
            node_x *next = cur->next;
            free(cur);
            cur = next;
        }
        a_node->next = NULL;

        a_node->next = b_rest;
        if (b_rest != NULL) b_rest->prev = a_node;

        line_x *del = a_line->nextline;
        while (del != NULL && del != b_line->nextline) {
            line_x *next = del->nextline;
            if (del != b_line) {
                node_x *n = del->dummynode;
                while (n != NULL) {
                    node_x *nn = n->next;
                    free(n);
                    n = nn;
                }
                free(del);
            } else {
                node_x *n = del->dummynode;
                while (n != NULL && n != b_rest) {
                    node_x *nn = n->next;
                    free(n);
                    n = nn;
                }
                free(del);
            }
            del = next;
        }

        a_line->nextline = b_line->nextline;
        if (b_line->nextline != NULL)
            b_line->nextline->prevline = a_line;
    }

    selection_clear();
}

void copySelection() {
    if (!selection_active) return;

    line_x *a_line = sel_start.line;
    node_x *a_node = sel_start.node;
    line_x *b_line = sel_end.line;
    node_x *b_node = sel_end.node;

    int a_before_b = 0;
    if (a_line == b_line)
        a_before_b = node_before(a_node, b_node);
    else
        a_before_b = line_before(a_line, b_line);

    if (!a_before_b) {
        line_x *tl = a_line;
        a_line = b_line;
        b_line = tl;
        node_x *tn = a_node;
        a_node = b_node;
        b_node = tn;
    }

    clipboard_clear();

    line_x *cur_line = a_line;
    while (cur_line != NULL) {
        node_x *start_node = (cur_line == a_line) ? a_node->next : cur_line->dummynode->next;
        node_x *end_node = (cur_line == b_line) ? b_node->next : NULL;

        int len = 0;
        node_x *tmp = start_node;
        while (tmp != end_node) {
            len++;
            tmp = tmp->next;
        }

        char *text = malloc(len + 1);
        int i = 0;
        tmp = start_node;
        while (tmp != end_node) {
            text[i++] = tmp->letter;
            tmp = tmp->next;
        }
        text[i] = '\0';

        ClipLine *cl = malloc(sizeof(ClipLine));
        cl->text = text;
        cl->next = NULL;

        if (clipboard == NULL) {
            clipboard = cl;
        } else {
            ClipLine *last = clipboard;
            while (last->next != NULL) last = last->next;
            last->next = cl;
        }

        if (cur_line == b_line) break;
        cur_line = cur_line->nextline;
    }
    selection_clear();
}

void cutSelection() {
    if (!selection_active) return;

    line_x *a_line = sel_start.line;
    node_x *a_node = sel_start.node;
    line_x *b_line = sel_end.line;
    node_x *b_node = sel_end.node;

    int a_before_b = 0;
    if (a_line == b_line)
        a_before_b = node_before(a_node, b_node);
    else
        a_before_b = line_before(a_line, b_line);

    if (!a_before_b) {
        line_x *tl = a_line; a_line = b_line; b_line = tl;
        node_x *tn = a_node; a_node = b_node; b_node = tn;
    }

    // Başlangıç pozisyonunu hesapla
    int start_line_idx = 0;
    line_x *tl = headline;
    while (tl != NULL && tl != a_line) { start_line_idx++; tl = tl->nextline; }

    int start_col = 0;
    node_x *nd = a_line->dummynode;
    while (nd != a_node) { start_col++; nd = nd->next; }

    // Kesilen metni hem clipboard'a hem string'e yaz
    clipboard_clear();
    int len = 0;
    line_x *cur_line = a_line;
    while (cur_line != NULL) {
        node_x *sn = (cur_line == a_line) ? a_node->next : cur_line->dummynode->next;
        node_x *en = (cur_line == b_line) ? b_node->next : NULL;
        node_x *tmp = sn;
        while (tmp != en) { len++; tmp = tmp->next; }
        if (cur_line != b_line) len++;
        if (cur_line == b_line) break;
        cur_line = cur_line->nextline;
    }

    char *cut_text = malloc(len + 1);
    int pos = 0;
    cur_line = a_line;
    while (cur_line != NULL) {
        node_x *sn = (cur_line == a_line) ? a_node->next : cur_line->dummynode->next;
        node_x *en = (cur_line == b_line) ? b_node->next : NULL;

        // Clipboard'a ekle
        int line_len = 0;
        node_x *tmp = sn;
        while (tmp != en) { line_len++; tmp = tmp->next; }
        char *clip_text = malloc(line_len + 1);
        int ci = 0;
        tmp = sn;
        while (tmp != en) { clip_text[ci++] = tmp->letter; cut_text[pos++] = tmp->letter; tmp = tmp->next; }
        clip_text[ci] = '\0';

        ClipLine *cl = malloc(sizeof(ClipLine));
        cl->text = clip_text;
        cl->next = NULL;
        if (clipboard == NULL) clipboard = cl;
        else { ClipLine *last = clipboard; while (last->next != NULL) last = last->next; last->next = cl; }

        if (cur_line != b_line) cut_text[pos++] = '\n';
        if (cur_line == b_line) break;
        cur_line = cur_line->nextline;
    }
    cut_text[pos] = '\0';

    // Encode et
    char header[64];
    snprintf(header, sizeof(header), "%d:%d:", start_line_idx, start_col);
    char *encoded = malloc(strlen(header) + len + 1);
    strcpy(encoded, header);
    memcpy(encoded + strlen(header), cut_text, len + 1);

    // Sil
    sel_start.line = a_line;
    sel_start.node = a_node;
    sel_end.line   = b_line;
    sel_end.node   = b_node;
    selection_active = 1;
    undo_enabled = 0;
    deleteSelection();
    undo_enabled = 1;

    undo_push(undo, OP_REPLACE, 0, 0, encoded);
    free(cut_text);
    free(encoded);
}

void pasteClipboard() {
    if (clipboard == NULL) return;

    // Başlangıç pozisyonunu hesapla
    int start_line_idx = 0;
    line_x *tl = headline;
    while (tl != NULL && tl != currentline) {
        start_line_idx++;
        tl = tl->nextline;
    }

    int start_col = 0;
    node_x *nd = currentline->dummynode;
    while (nd != cursor) {
        start_col++;
        nd = nd->next;
    }

    undo_enabled = 0;

    ClipLine *cl = clipboard;
    while (cl != NULL) {
        for (int i = 0; cl->text[i] != '\0'; i++)
            letterEntry(cl->text[i]);
        if (cl->next != NULL)
            addnewline();
        cl = cl->next;
    }

    undo_enabled = 1;

    // Bitiş pozisyonunu hesapla
    int end_line_idx = 0;
    tl = headline;
    while (tl != NULL && tl != currentline) {
        end_line_idx++;
        tl = tl->nextline;
    }

    int end_col = 0;
    nd = currentline->dummynode;
    while (nd != cursor) {
        end_col++;
        nd = nd->next;
    }

    // "start_line:start_col:end_line:end_col" formatında encode et
    char encoded[128];
    snprintf(encoded, sizeof(encoded), "%d:%d:%d:%d",
             start_line_idx, start_col, end_line_idx, end_col);

    undo_push(undo, OP_REPLACE, 0, 0, encoded);
}

void performUndo() {
    if (undo == NULL) return;
    UndoRecord *rec = undo_pop(undo);
    if (rec == NULL) return;

    undo_enabled = 0;

    switch (rec->type) {
        case OP_INSERT_CHAR: {
            line_x *target_line = headline;
            for (int i = 0; i < rec->line_index && target_line != NULL; i++)
                target_line = target_line->nextline;
            if (target_line == NULL) break;

            currentline = target_line;
            cursor = currentline->dummynode;
            for (int i = 0; i < rec->col; i++)
                if (cursor->next != NULL) cursor = cursor->next;
            if (cursor->next != NULL) {
                node_x *del = cursor->next;
                cursor->next = del->next;
                if (del->next != NULL) del->next->prev = cursor;
                free(del);
            }
            break;
        }
        case OP_DELETE_CHAR: {
            line_x *target_line = headline;
            for (int i = 0; i < rec->line_index && target_line != NULL; i++)
                target_line = target_line->nextline;
            if (target_line == NULL) break;

            currentline = target_line;
            cursor = currentline->dummynode;
            for (int i = 0; i < rec->col - 1 && cursor->next != NULL; i++)
                cursor = cursor->next;
            if (rec->text != NULL)
                letterEntry(rec->text[0]);
            break;
        }
        case OP_INSERT_LINE: {
            line_x *target_line = headline;
            for (int i = 0; i < rec->line_index && target_line != NULL; i++)
                target_line = target_line->nextline;
            if (target_line == NULL) break;

            currentline = target_line->nextline;
            if (currentline != NULL) {
                cursor = currentline->dummynode;
                mergeLines();
            }
            break;
        }
        case OP_DELETE_LINE: {
            line_x *target_line = headline;
            for (int i = 0; i < rec->line_index && target_line != NULL; i++)
                target_line = target_line->nextline;
            if (target_line == NULL) break;

            currentline = target_line;
            cursor = currentline->dummynode;
            while (cursor->next != NULL) cursor = cursor->next;
            addnewline();
            break;
        }
        case OP_REPLACE: {
            // Format kontrolü: kaç tane ':' var
            int colon_count = 0;
            for (int i = 0; rec->text[i] != '\0'; i++)
                if (rec->text[i] == ':') colon_count++;

            if (colon_count >= 3) {
                // Yapıştırma geri al: "start_line:start_col:end_line:end_col"
                int sl, sc, el, ec;
                sscanf(rec->text, "%d:%d:%d:%d", &sl, &sc, &el, &ec);

                line_x *start_line = headline;
                for (int i = 0; i < sl && start_line != NULL; i++)
                    start_line = start_line->nextline;
                if (start_line == NULL) break;

                line_x *end_line = headline;
                for (int i = 0; i < el && end_line != NULL; i++)
                    end_line = end_line->nextline;
                if (end_line == NULL) break;

                node_x *start_node = start_line->dummynode;
                for (int i = 0; i < sc; i++)
                    if (start_node->next != NULL) start_node = start_node->next;

                node_x *end_node = end_line->dummynode;
                for (int i = 0; i < ec; i++)
                    if (end_node->next != NULL) end_node = end_node->next;

                sel_start.line = start_line;
                sel_start.node = start_node;
                sel_end.line = end_line;
                sel_end.node = end_node;
                selection_active = 1;
                deleteSelection();
            } else {
                // Kesme geri al: "start_line:start_col:metin"
                int sl, sc;
                char cut_text[4096];
                sscanf(rec->text, "%d:%d:%[^\n]", &sl, &sc, cut_text);
                // Metindeki \n karakterlerini gerçek newline'a çevir
                // (zaten gerçek \n olarak saklandı)

                line_x *target_line = headline;
                for (int i = 0; i < sl && target_line != NULL; i++)
                    target_line = target_line->nextline;
                if (target_line == NULL) break;

                currentline = target_line;
                cursor = currentline->dummynode;
                for (int i = 0; i < sc; i++)
                    if (cursor->next != NULL) cursor = cursor->next;

                // Metni geri yapıştır
                for (int i = 0; rec->text[i] != '\0'; i++) {
                    // "sl:sc:" prefix'ini atla
                    int prefix_len = 0;
                    char tmp[64];
                    snprintf(tmp, sizeof(tmp), "%d:%d:", sl, sc);
                    prefix_len = strlen(tmp);

                    for (int j = prefix_len; rec->text[j] != '\0'; j++) {
                        if (rec->text[j] == '\n') {
                            addnewline();
                        } else {
                            letterEntry(rec->text[j]);
                        }
                    }
                    break;
                }
            }
            break;
        }
        default: break;
    }

    undo_enabled = 1;
    undo_record_destroy(rec);
}

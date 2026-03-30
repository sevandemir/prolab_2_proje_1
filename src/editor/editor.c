#include "editor.h"
#include "buffer.h"
#include "selection.h"
#include "../undo/undo_stack.h"
#include "../terminal/terminal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "cursor.h"

/* ================================================================
   PANO — basit statik tampon, bağlı liste YOK
   ================================================================ */
#define CLIPBOARD_MAX 65536
static char clipboard_buf[CLIPBOARD_MAX] = {0};
static int  clipboard_len = 0;          /* 0 ise pano boş */

/* ================================================================
   YARDIMCI: sıralama
   ================================================================ */
static int node_before(node_x *a, node_x *b) {
    node_x *t = a;
    while (t) { if (t == b) return 1; t = t->next; }
    return 0;
}
static int line_before(line_x *a, line_x *b) {
    line_x *t = a;
    while (t) { if (t == b) return 1; t = t->nextline; }
    return 0;
}

/* Seçimin başlangıç/bitiş noktalarını belgede doğru sıraya koyar */
static void normalize_sel(line_x **al, node_x **an, line_x **bl, node_x **bn) {
    int a_first = (*al == *bl) ? node_before(*an, *bn) : line_before(*al, *bl);
    if (!a_first) {
        line_x *tl = *al; *al = *bl; *bl = tl;
        node_x *tn = *an; *an = *bn; *bn = tn;
    }
}

/* Seçili metni clipboard_buf'a yazar; '\n' = satır sonu */
static void selection_to_clipboard(
        line_x *al, node_x *an, line_x *bl, node_x *bn) {
    clipboard_len = 0;
    line_x *cl = al;
    while (cl && clipboard_len < CLIPBOARD_MAX - 1) {
        node_x *sn = (cl == al) ? an->next : cl->dummynode->next;
        node_x *en = (cl == bl) ? bn->next : NULL;
        for (node_x *t = sn; t != en && clipboard_len < CLIPBOARD_MAX - 1; t = t->next)
            clipboard_buf[clipboard_len++] = t->letter;
        if (cl != bl && clipboard_len < CLIPBOARD_MAX - 1)
            clipboard_buf[clipboard_len++] = '\n';
        if (cl == bl) break;
        cl = cl->nextline;
    }
    clipboard_buf[clipboard_len] = '\0';
}

/* ================================================================
   SEÇME / SİLME
   ================================================================ */
void deleteSelection() {
    if (!selection_active) return;

    line_x *al = sel_start.line, *bl = sel_end.line;
    node_x *an = sel_start.node, *bn = sel_end.node;
    normalize_sel(&al, &an, &bl, &bn);

    currentline = al;
    cursor      = an;

    if (al == bl) {
        node_x *c = an->next;
        while (c && c != bn->next) {
            node_x *nx = c->next;
            an->next = nx;
            if (nx) nx->prev = an;
            free(c); c = nx;
        }
    } else {
        node_x *b_rest = bn->next;
        node_x *c = an->next;
        while (c) { node_x *nx = c->next; free(c); c = nx; }
        an->next = b_rest;
        if (b_rest) b_rest->prev = an;

        line_x *del = al->nextline;
        while (del && del != bl->nextline) {
            line_x *nx = del->nextline;
            node_x *n  = del->dummynode;
            while (n) { node_x *nn = n->next; free(n); n = nn; }
            free(del); del = nx;
        }
        al->nextline = bl->nextline;
        if (bl->nextline) bl->nextline->prevline = al;
    }
    selection_clear();
}

/* ================================================================
   KOPYALAMA  (CTRL+C)
   ================================================================ */
void copySelection() {
    if (!selection_active) return;

    line_x *al = sel_start.line, *bl = sel_end.line;
    node_x *an = sel_start.node, *bn = sel_end.node;
    normalize_sel(&al, &an, &bl, &bn);

    selection_to_clipboard(al, an, bl, bn);
    term_clipboard_set(clipboard_buf, clipboard_len); /* Windows panoya gönder */
    /* Seçimi koru — vurgulanmış halde kalsın */
}

/* ================================================================
   KESME  (CTRL+X)
   ================================================================ */
void cutSelection() {
    if (!selection_active) return;

    line_x *al = sel_start.line, *bl = sel_end.line;
    node_x *an = sel_start.node, *bn = sel_end.node;
    normalize_sel(&al, &an, &bl, &bn);

    /* Başlangıç konumu */
    int sl = 0; line_x *tl = headline;
    while (tl && tl != al) { sl++; tl = tl->nextline; }
    int sc = 0; node_x *nd = al->dummynode;
    while (nd != an)        { sc++; nd = nd->next; }

    /* Panoya kopyala */
    selection_to_clipboard(al, an, bl, bn);
    term_clipboard_set(clipboard_buf, clipboard_len); /* Windows panoya gönder */

    /* Undo için metni encode et: "CUT\0sl\0sc\0metin" → basit prefix */
    /* Format: sl:sc: + metin (metin içinde \n olabilir, sorun değil) */
    int tlen = clipboard_len;
    char *enc = malloc(32 + tlen + 1);
    int plen  = snprintf(enc, 32, "%d:%d:", sl, sc);
    memcpy(enc + plen, clipboard_buf, tlen + 1);

    /* Seç ve sil */
    sel_start.line = al; sel_start.node = an;
    sel_end.line   = bl; sel_end.node   = bn;
    selection_active = 1;
    undo_enabled = 0;
    deleteSelection();
    undo_enabled = 1;

    undo_push(undo, OP_REPLACE, 0, 0, enc);
    free(enc);
}

/* ================================================================
   YAPIŞTIRMA  (CTRL+V) — iç panodan
   ================================================================ */
void pasteClipboard() {
    /* Windows panosunu önce iç buffer'a yükle (diğer uygulamadan yapıştırma) */
    {
        static char sys_buf[CLIPBOARD_MAX];
        int slen = term_clipboard_get(sys_buf, CLIPBOARD_MAX);
        if (slen > 0) {
            memcpy(clipboard_buf, sys_buf, slen);
            clipboard_buf[slen] = '\0';
            clipboard_len = slen;
        }
    }
    if (clipboard_len == 0) return;

    /* Başlangıç konumu */
    int sl = 0; line_x *tl = headline;
    while (tl && tl != currentline) { sl++; tl = tl->nextline; }
    int sc = 0; node_x *nd = currentline->dummynode;
    while (nd != cursor) { sc++; nd = nd->next; }

    undo_enabled = 0;
    for (int i = 0; i < clipboard_len; i++) {
        if (clipboard_buf[i] == '\n') addnewline();
        else                          letterEntry(clipboard_buf[i]);
    }
    undo_enabled = 1;

    /* Bitiş konumu */
    int el = 0; tl = headline;
    while (tl && tl != currentline) { el++; tl = tl->nextline; }
    int ec = 0; nd = currentline->dummynode;
    while (nd != cursor) { ec++; nd = nd->next; }

    char enc[128];
    snprintf(enc, sizeof(enc), "PASTE:%d:%d:%d:%d", sl, sc, el, ec);
    undo_push(undo, OP_REPLACE, 0, 0, enc);
}

/* ================================================================
   GERI AL (CTRL+Z)
   ================================================================ */
void performUndo() {
    if (!undo) return;
    UndoRecord *rec = undo_pop(undo);
    if (!rec) return;

    undo_enabled = 0;

    switch (rec->type) {
        case OP_INSERT_CHAR: {
            line_x *t = headline;
            for (int i = 0; i < rec->line_index && t; i++) t = t->nextline;
            if (!t) break;
            currentline = t;
            cursor = currentline->dummynode;
            for (int i = 0; i < rec->col && cursor->next; i++) cursor = cursor->next;
            /* Leading byte'i ve ardindan gelen UTF-8 devam byte'larini atomik sil */
            while (cursor->next) {
                node_x *d = cursor->next;
                cursor->next = d->next;
                if (d->next) d->next->prev = cursor;
                free(d);
                /* Sonraki byte devam byte'i degilse dur */
                if (!cursor->next || !IS_CONT_BYTE((unsigned char)cursor->next->letter))
                    break;
            }
            break;
        }
        case OP_DELETE_CHAR: {
            line_x *t = headline;
            for (int i = 0; i < rec->line_index && t; i++) t = t->nextline;
            if (!t) break;
            currentline = t;
            cursor = currentline->dummynode;
            for (int i = 0; i < rec->col - 1 && cursor->next; i++) cursor = cursor->next;
            /* Tum UTF-8 byte'larini geri ekle (tek veya cok baytli karakter) */
            if (rec->text) {
                for (int i = 0; (unsigned char)rec->text[i] != 0; i++)
                    letterEntry(rec->text[i]);
            }
            break;
        }
        case OP_INSERT_LINE: {
            line_x *t = headline;
            for (int i = 0; i < rec->line_index && t; i++) t = t->nextline;
            if (!t) break;
            currentline = t->nextline;
            if (currentline) { cursor = currentline->dummynode; mergeLines(); }
            break;
        }
        case OP_DELETE_LINE: {
            line_x *t = headline;
            for (int i = 0; i < rec->line_index && t; i++) t = t->nextline;
            if (!t) break;
            currentline = t;
            cursor = currentline->dummynode;
            while (cursor->next) cursor = cursor->next;
            addnewline();
            break;
        }
        case OP_REPLACE: {
            if (!rec->text) break;

            if (strncmp(rec->text, "PASTE:", 6) == 0) {
                /* Yapıştırmayı geri al: yapıştırılan bölgeyi sil */
                int sl, sc, el, ec;
                sscanf(rec->text + 6, "%d:%d:%d:%d", &sl, &sc, &el, &ec);

                line_x *sl_line = headline;
                for (int i = 0; i < sl && sl_line; i++) sl_line = sl_line->nextline;
                line_x *el_line = headline;
                for (int i = 0; i < el && el_line; i++) el_line = el_line->nextline;
                if (!sl_line || !el_line) break;

                node_x *sn = sl_line->dummynode;
                for (int i = 0; i < sc && sn->next; i++) sn = sn->next;
                node_x *en = el_line->dummynode;
                for (int i = 0; i < ec && en->next; i++) en = en->next;

                sel_start.line = sl_line; sel_start.node = sn;
                sel_end.line   = el_line; sel_end.node   = en;
                selection_active = 1;
                deleteSelection();

            } else {
                /* Kesmeyi geri al: "sl:sc:metin" → metni geri koy */
                int sl, sc;
                const char *p = rec->text;
                sl = atoi(p); p = strchr(p, ':'); if (!p) break; p++;
                sc = atoi(p); p = strchr(p, ':'); if (!p) break; p++;

                line_x *t = headline;
                for (int i = 0; i < sl && t; i++) t = t->nextline;
                if (!t) break;
                currentline = t;
                cursor = currentline->dummynode;
                for (int i = 0; i < sc && cursor->next; i++) cursor = cursor->next;

                while (*p) {
                    if (*p == '\n') addnewline();
                    else            letterEntry(*p);
                    p++;
                }
            }
            break;
        }
        default: break;
    }

    undo_enabled = 1;
    undo_record_destroy(rec);
}

/* ================================================================
   YARDIMCI
   ================================================================ */
void deleteNode(node_x *n) {
    if (!n || !n->prev) return;
    node_x *prev = n->prev, *next = n->next;
    prev->next = next;
    if (next) next->prev = prev;
    free(n);
}

void deleteWordLeft() {
    if (!cursor || cursor == currentline->dummynode) {
        /* If at the start of a line, delete the newline character (merge lines) */
        if (currentline->prevline != NULL) {
            deleteLetter();
        }
        return;
    }
    
    // Delete trailing spaces first
    while (cursor != currentline->dummynode && isspace((unsigned char)cursor->letter)) {
        deleteLetter();
    }
    
    // Then delete non-spaces until we hit a space or start of line
    while (cursor != currentline->dummynode && !isspace((unsigned char)cursor->letter)) {
        deleteLetter();
    }
}
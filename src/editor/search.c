#include "search.h"
#include "../editor/buffer.h"
#include "../editor/cursor.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int search_active  = 0;
int replace_active = 0;
char search_query[256]  = {0};
char replace_query[256] = {0};
SearchMatch *search_matches = NULL;
SearchMatch *current_match  = NULL;

static void matches_clear() {
    SearchMatch *cur = search_matches;
    while (cur != NULL) {
        SearchMatch *next = cur->next;
        free(cur);
        cur = next;
    }
    search_matches = NULL;
    current_match  = NULL;
}

static int str_match_ci(const char *text, int text_len, const char *query, int query_len) {
    if (query_len > text_len) return 0;
    for (int i = 0; i < query_len; i++) {
        if (tolower((unsigned char)text[i]) != tolower((unsigned char)query[i]))
            return 0;
    }
    return 1;
}

void search_find(const char *query) {
    matches_clear();
    if (query == NULL || query[0] == '\0') return;

    int query_len = strlen(query);
    int line_idx  = 0;
    line_x *line  = headline;

    while (line != NULL) {
        // Satırı char dizisine dönüştür
        int len = 0;
        node_x *tmp = line->dummynode->next;
        while (tmp != NULL) { len++; tmp = tmp->next; }

        char *buf = malloc(len + 1);
        int i = 0;
        tmp = line->dummynode->next;
        while (tmp != NULL) { buf[i++] = tmp->letter; tmp = tmp->next; }
        buf[len] = '\0';

        // Satırda eşleşme ara
        for (int col = 0; col <= len - query_len; col++) {
            if (str_match_ci(buf + col, len - col, query, query_len)) {
                SearchMatch *m = malloc(sizeof(SearchMatch));
                m->line_index = line_idx;
                m->col        = col;
                m->len        = query_len;
                m->next       = NULL;

                if (search_matches == NULL) {
                    search_matches = m;
                    current_match  = m;
                } else {
                    SearchMatch *last = search_matches;
                    while (last->next != NULL) last = last->next;
                    last->next = m;
                }
            }
        }

        free(buf);
        line = line->nextline;
        line_idx++;
    }
}

void search_clear() {
    matches_clear();
    search_active  = 0;
    replace_active = 0;
    search_query[0]  = '\0';
    replace_query[0] = '\0';
}

void search_next() {
    if (current_match == NULL) return;
    if (current_match->next != NULL)
        current_match = current_match->next;
    else
        current_match = search_matches; // başa dön

    // İmleci eşleşmeye götür
    line_x *line = headline;
    for (int i = 0; i < current_match->line_index && line != NULL; i++)
        line = line->nextline;
    if (line == NULL) return;

    currentline = line;
    cursor = currentline->dummynode;
    for (int i = 0; i < current_match->col; i++)
        if (cursor->next != NULL) cursor = cursor->next;
}

void search_prev() {
    if (current_match == NULL) return;

    // Bir öncekini bul
    if (current_match == search_matches) {
        // Sona git
        SearchMatch *last = search_matches;
        while (last->next != NULL) last = last->next;
        current_match = last;
    } else {
        SearchMatch *prev = search_matches;
        while (prev->next != current_match) prev = prev->next;
        current_match = prev;
    }

    line_x *line = headline;
    for (int i = 0; i < current_match->line_index && line != NULL; i++)
        line = line->nextline;
    if (line == NULL) return;

    currentline = line;
    cursor = currentline->dummynode;
    for (int i = 0; i < current_match->col; i++)
        if (cursor->next != NULL) cursor = cursor->next;
}

int search_contains(int line_index, int col) {
    if (!search_active) return 0;
    SearchMatch *m = search_matches;
    while (m != NULL) {
        if (m->line_index == line_index && col >= m->col && col < m->col + m->len)
            return 1;
        m = m->next;
    }
    return 0;
}

void search_replace_all(const char *query, const char *replacement) {
    if (query == NULL || query[0] == '\0') return;

    search_find(query);
    if (search_matches == NULL) return;

    int rep_len   = strlen(replacement);
    int query_len = strlen(query);

    // Eşleşmeleri diziye al
    int count = 0;
    SearchMatch *m = search_matches;
    while (m != NULL) { count++; m = m->next; }

    SearchMatch **arr = malloc(count * sizeof(SearchMatch *));
    m = search_matches;
    for (int i = 0; i < count; i++) { arr[i] = m; m = m->next; }

    // Sondan başa doğru değiştir
    for (int i = count - 1; i >= 0; i--) {
        line_x *line = headline;
        for (int j = 0; j < arr[i]->line_index && line != NULL; j++)
            line = line->nextline;
        if (line == NULL) continue;

        node_x *cur = line->dummynode;
        for (int j = 0; j < arr[i]->col; j++)
            if (cur->next != NULL) cur = cur->next;

        // query_len kadar sil
        for (int j = 0; j < query_len; j++) {
            if (cur->next != NULL) {
                node_x *del = cur->next;
                cur->next = del->next;
                if (del->next != NULL) del->next->prev = cur;
                free(del);
            }
        }

        // replacement ekle
        currentline = line;
        cursor = cur;
        for (int j = 0; j < rep_len; j++) {
            node_x *newnode = malloc(sizeof(node_x));
            newnode->letter = replacement[j];
            newnode->next   = cursor->next;
            newnode->prev   = cursor;
            if (cursor->next != NULL) cursor->next->prev = newnode;
            cursor->next = newnode;
            cursor = newnode;
        }
    }

    free(arr);
    matches_clear();
}
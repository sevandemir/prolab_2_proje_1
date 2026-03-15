#ifndef SEARCH_H
#define SEARCH_H

typedef struct SearchMatch {
    int line_index;
    int col;
    int len;
    struct SearchMatch *next;
} SearchMatch;

extern int search_active;
extern int replace_active;
extern char search_query[256];
extern char replace_query[256];
extern SearchMatch *search_matches;
extern SearchMatch *current_match;

void search_find(const char *query);
void search_clear();
void search_next();
void search_prev();
void search_replace_all(const char *query, const char *replacement);
int  search_contains(int line_index, int col);

#endif
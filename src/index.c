#include <stdlib.h>
#include <string.h>

#include "index.h"

#define HASH_SIZE 4096

static WordEntry* g_table[HASH_SIZE];

static unsigned long hash_word(const char* s) {
    unsigned long h = 5381UL;
    int c;
    while ((c = (unsigned char)*s++) != 0) h = ((h << 5) + h) + (unsigned long)c;
    return h;
}

static WordEntry* find_entry(const char* word, unsigned long* out_bucket) {
    unsigned long b = hash_word(word) % (unsigned long)HASH_SIZE;
    if (out_bucket) *out_bucket = b;
    WordEntry* cur = g_table[b];
    while (cur) {
        if (strcmp(cur->word, word) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

void index_init(void) {
    for (int i = 0; i < HASH_SIZE; i++) g_table[i] = NULL;
}

void index_free(void) {
    for (int i = 0; i < HASH_SIZE; i++) {
        WordEntry* e = g_table[i];
        while (e) {
            WordEntry* next = e->next;
            free(e);
            e = next;
        }
        g_table[i] = NULL;
    }
}

void insert_word(char* word, int docID) {
    if (!word || !word[0] || docID <= 0) return;
    unsigned long b = 0;
    if (find_entry(word, &b)) return;

    WordEntry* e = (WordEntry*)malloc(sizeof(WordEntry));
    if (!e) return;
    memset(e, 0, sizeof(WordEntry));
    strncpy(e->word, word, WORD_MAX_LEN - 1);
    e->next = g_table[b];
    g_table[b] = e;
}

void insert_term(const char* word, int docID, int position) {
    (void)position;
    insert_word((char*)word, docID);
}

Posting* get_postings(char* word) {
    if (!word || !word[0]) return NULL;
    WordEntry* cur = find_entry(word, NULL);
    return cur ? cur->postingList : NULL;
}

int get_document_frequency(const char* word) {
    if (!word || !word[0]) return 0;
    WordEntry* cur = find_entry(word, NULL);
    return cur ? cur->documentFrequency : 0;
}

int get_vocabulary_size(void) {
    int total = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        for (WordEntry* e = g_table[i]; e; e = e->next) total++;
    }
    return total;
}

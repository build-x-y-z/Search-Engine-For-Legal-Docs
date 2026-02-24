#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_loader.h"
#include "index.h"
#include "tokenizer.h"
#include "trie.h"

#define MAX_LINE 8192

static int g_doc_count = 0;
static int g_doc_cap = 0;
static char** g_doc_labels = NULL;
static char** g_doc_texts = NULL;
static long long g_total_tokens = 0;

static int ensure_doc_capacity(int needed) {
    if (needed <= g_doc_cap) return 1;
    int new_cap = (g_doc_cap > 0) ? g_doc_cap : 64;
    while (new_cap < needed) new_cap *= 2;

    char** labels = (char**)realloc(g_doc_labels, (size_t)(new_cap + 1) * sizeof(char*));
    if (!labels) return 0;
    char** texts = (char**)realloc(g_doc_texts, (size_t)(new_cap + 1) * sizeof(char*));
    if (!texts) return 0;

    for (int i = g_doc_cap + 1; i <= new_cap; i++) {
        labels[i] = NULL;
        texts[i] = NULL;
    }

    g_doc_labels = labels;
    g_doc_texts = texts;
    g_doc_cap = new_cap;
    return 1;
}

char* normalize_token(char* token) {
    if (!token) return NULL;
    int w = 0;
    for (int i = 0; token[i] != '\0'; i++) {
        unsigned char ch = (unsigned char)token[i];
        if (isalnum(ch) || ch == '_') token[w++] = (char)tolower(ch);
    }
    token[w] = '\0';
    return token;
}

int tokenize_text(const char* text, char tokens[][WORD_MAX_LEN], int max_tokens) {
    if (!text || !tokens || max_tokens <= 0) return 0;
    int out = 0;
    char cur[WORD_MAX_LEN];
    int len = 0;

    for (int i = 0; ; i++) {
        unsigned char ch = (unsigned char)text[i];
        int done = (ch == '\0');
        if (!done && (isalnum(ch) || ch == '_')) {
            if (len < WORD_MAX_LEN - 1) cur[len++] = (char)tolower(ch);
        } else {
            if (len > 0) {
                cur[len] = '\0';
                normalize_token(cur);
                strncpy(tokens[out], cur, WORD_MAX_LEN - 1);
                tokens[out][WORD_MAX_LEN - 1] = '\0';
                out++;
                len = 0;
                if (out >= max_tokens) return out;
            }
            if (done) break;
        }
    }
    return out;
}

void tokenize_free_tokens(char** tokens, int count) {
    if (!tokens) return;
    for (int i = 0; i < count; i++) free(tokens[i]);
    free(tokens);
}

int tokenize_text_dynamic(const char* text, char*** out_tokens, int* out_count) {
    if (!out_tokens || !out_count) return 0;
    *out_tokens = NULL;
    *out_count = 0;
    if (!text) return 1;

    int cap = 64;
    char** toks = (char**)malloc((size_t)cap * sizeof(char*));
    if (!toks) return 0;
    int n = 0;
    char cur[WORD_MAX_LEN];
    int len = 0;

    for (int i = 0; ; i++) {
        unsigned char ch = (unsigned char)text[i];
        int done = (ch == '\0');
        if (!done && (isalnum(ch) || ch == '_')) {
            if (len < WORD_MAX_LEN - 1) cur[len++] = (char)tolower(ch);
        } else {
            if (len > 0) {
                cur[len] = '\0';
                normalize_token(cur);
                if (cur[0] != '\0') {
                    if (n >= cap) {
                        cap *= 2;
                        char** grown = (char**)realloc(toks, (size_t)cap * sizeof(char*));
                        if (!grown) {
                            tokenize_free_tokens(toks, n);
                            return 0;
                        }
                        toks = grown;
                    }
                    toks[n] = (char*)malloc(strlen(cur) + 1);
                    if (!toks[n]) {
                        tokenize_free_tokens(toks, n);
                        return 0;
                    }
                    strcpy(toks[n], cur);
                    n++;
                }
                len = 0;
            }
            if (done) break;
        }
    }

    *out_tokens = toks;
    *out_count = n;
    return 1;
}

static void free_docs(void) {
    for (int i = 1; i <= g_doc_count; i++) {
        free(g_doc_labels ? g_doc_labels[i] : NULL);
        free(g_doc_texts ? g_doc_texts[i] : NULL);
    }
    g_doc_count = 0;
    g_total_tokens = 0;
}

static void index_text_with_label(const char* label, const char* text) {
    if (!text || !text[0]) return;
    if (!ensure_doc_capacity(g_doc_count + 1)) return;

    g_doc_count++;
    int doc_id = g_doc_count;

    if (label && label[0]) {
        g_doc_labels[doc_id] = (char*)malloc(strlen(label) + 1);
        if (g_doc_labels[doc_id]) strcpy(g_doc_labels[doc_id], label);
    }

    g_doc_texts[doc_id] = (char*)malloc(strlen(text) + 1);
    if (g_doc_texts[doc_id]) strcpy(g_doc_texts[doc_id], text);

    char** toks = NULL;
    int n = 0;
    if (!tokenize_text_dynamic(text, &toks, &n)) return;
    for (int i = 0; i < n; i++) {
        insert_word(toks[i], doc_id);
        insert_trie(toks[i]);
        g_total_tokens++;
    }
    tokenize_free_tokens(toks, n);
}

static void loader_sink(const char* label, const char* text, void* user) {
    (void)user;
    index_text_with_label(label, text);
}

void tokenize_document(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return;
    free_docs();

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char* colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        char* body = colon + 1;
        while (*body == ' ' || *body == '\t') body++;
        body[strcspn(body, "\r\n")] = '\0';
        index_text_with_label(line, body);
    }
    fclose(f);
}

void tokenize_directory(const char* root_dir) {
    free_docs();
    file_loader_set_sink(loader_sink, NULL);
    (void)load_documents_from_directory(root_dir);
}

int get_document_count(void) {
    return g_doc_count;
}

const char* get_document_label(int docID) {
    if (docID <= 0 || docID > g_doc_count) return NULL;
    return g_doc_labels ? g_doc_labels[docID] : NULL;
}

const char* get_document_text(int docID) {
    if (docID <= 0 || docID > g_doc_count) return NULL;
    return g_doc_texts ? g_doc_texts[docID] : NULL;
}

long long get_total_tokens_indexed(void) {
    return g_total_tokens;
}

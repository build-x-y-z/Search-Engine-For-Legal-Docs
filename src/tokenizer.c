#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "tokenizer.h"

static int g_doc_count = 0;

char* normalize_token(char* token) {
    if (!token) return NULL;
    for (int i = 0; token[i] != '\0'; i++) {
        token[i] = (char)tolower((unsigned char)token[i]);
    }
    return token;
}

int tokenize_text(const char* text, char tokens[][WORD_MAX_LEN], int max_tokens) {
    if (!text || !tokens || max_tokens <= 0) return 0;
    int out = 0;
    char current[WORD_MAX_LEN];
    int len = 0;

    for (int i = 0; ; i++) {
        unsigned char ch = (unsigned char)text[i];
        int done = (ch == '\0');

        if (!done && isalpha(ch)) {
            if (len < WORD_MAX_LEN - 1) current[len++] = (char)tolower(ch);
        } else {
            if (len > 0) {
                current[len] = '\0';
                strncpy(tokens[out], current, WORD_MAX_LEN - 1);
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

int tokenize_text_dynamic(const char* text, char*** out_tokens, int* out_count) {
    (void)text;
    if (!out_tokens || !out_count) return 0;
    *out_tokens = NULL;
    *out_count = 0;
    return 1;
}

void tokenize_free_tokens(char** tokens, int count) {
    (void)tokens;
    (void)count;
}

void tokenize_document(const char* filename) {
    (void)filename;
    g_doc_count = 0;
}

void tokenize_directory(const char* root_dir) {
    (void)root_dir;
    g_doc_count = 0;
}

int get_document_count(void) {
    return g_doc_count;
}

const char* get_document_label(int docID) {
    (void)docID;
    return NULL;
}

const char* get_document_text(int docID) {
    (void)docID;
    return NULL;
}

long long get_total_tokens_indexed(void) {
    return 0;
}

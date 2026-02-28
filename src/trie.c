#include <stdlib.h>

#include "trie.h"

static TrieNode* g_root = NULL;

static TrieNode* trie_node_create(void) {
    TrieNode* node = (TrieNode*)malloc(sizeof(TrieNode));
    if (!node) return NULL;
    for (int i = 0; i < 26; i++) node->children[i] = NULL;
    node->isEndOfWord = 0;
    return node;
}

void trie_init(void) {
    if (!g_root) g_root = trie_node_create();
}

void trie_free(void) {
    /* Recursive cleanup will be added with autocomplete traversal. */
    g_root = NULL;
}

void insert_trie(char* word) {
    if (!word || !word[0]) return;
    if (!g_root) trie_init();

    TrieNode* cur = g_root;
    for (int i = 0; word[i] != '\0'; i++) {
        char c = word[i];
        if (c < 'a' || c > 'z') continue;
        int idx = c - 'a';
        if (!cur->children[idx]) {
            cur->children[idx] = trie_node_create();
            if (!cur->children[idx]) return;
        }
        cur = cur->children[idx];
    }
    cur->isEndOfWord = 1;
}

void autocomplete(char* prefix) {
    (void)prefix;
}

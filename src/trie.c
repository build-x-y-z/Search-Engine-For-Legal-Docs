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
    /* Temporary no-op; recursive cleanup added later. */
    g_root = NULL;
}

void insert_trie(char* word) {
    (void)word;
}

void autocomplete(char* prefix) {
    (void)prefix;
}

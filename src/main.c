#include <stdio.h>
#include <string.h>

#include "tokenizer.h"
#include "index.h"
#include "trie.h"

int main(void) {
    index_init();
    trie_init();
    tokenize_document("data/documents.txt");

    printf("Search Engine Ready\n");
    printf("Enter query (or quit): ");

    char query[256];
    while (fgets(query, sizeof(query), stdin)) {
        query[strcspn(query, "\r\n")] = '\0';
        if (strcmp(query, "quit") == 0) break;
        printf("Query received: %s\n", query);
        printf("Enter query (or quit): ");
    }

    index_free();
    trie_free();
    return 0;
}

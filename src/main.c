#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"
#include "index.h"
#include "search.h"
#include "trie.h"

int main(void) {
    index_init();
    trie_init();
    tokenize_document("data/documents.txt");

    printf("Search Engine Ready\n");
    printf("Commands: <query>, ac <prefix>, stats, quit\n");
    printf("Enter query (or quit): ");

    char query[256];
    while (fgets(query, sizeof(query), stdin)) {
        query[strcspn(query, "\r\n")] = '\0';
        if (strcmp(query, "quit") == 0) break;
        if (strncmp(query, "ac ", 3) == 0) {
            char* prefix = query + 3;
            while (*prefix == ' ') prefix++;
            printf("Suggestions:\n");
            autocomplete(prefix);
            printf("Enter query (or quit): ");
            continue;
        }
        if (strcmp(query, "stats") == 0) {
            printf("Documents indexed: %d\n", get_document_count());
            printf("Vocabulary size: %d\n", get_vocabulary_size());
            printf("Total tokens indexed: %lld\n", get_total_tokens_indexed());
            printf("Enter query (or quit): ");
            continue;
        }
        SearchResult* results = NULL;
        int count = 0;
        if (!search_query(query, &results, &count)) {
            printf("Search failed\n");
            printf("Enter query (or quit): ");
            continue;
        }
        if (count == 0) {
            printf("No results\n");
            free(results);
            printf("Enter query (or quit): ");
            continue;
        }
        printf("Results:\n");
        for (int i = 0; i < count && i < 10; i++) {
            const char* label = get_document_label(results[i].docID);
            if (!label) label = "(unknown)";
            printf("%s (score %.2f)\n", label, results[i].score);
        }
        free(results);
        printf("Enter query (or quit): ");
    }

    index_free();
    trie_free();
    return 0;
}

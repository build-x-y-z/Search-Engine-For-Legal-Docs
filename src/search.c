#include <stdlib.h>

#include "search.h"
#include "index.h"
#include "tokenizer.h"

static int search_and_terms(char** terms, int term_count, SearchResult** out_results, int* out_count) {
    *out_results = NULL;
    *out_count = 0;
    if (term_count <= 0) return 1;

    Posting** lists = (Posting**)malloc((size_t)term_count * sizeof(Posting*));
    Posting** iters = (Posting**)malloc((size_t)term_count * sizeof(Posting*));
    if (!lists || !iters) {
        free(lists);
        free(iters);
        return 0;
    }

    for (int i = 0; i < term_count; i++) {
        lists[i] = get_postings(terms[i]);
        iters[i] = lists[i];
        if (!lists[i]) {
            free(lists);
            free(iters);
            return 1;
        }
    }

    int cap = 32;
    SearchResult* results = (SearchResult*)malloc((size_t)cap * sizeof(SearchResult));
    if (!results) {
        free(lists);
        free(iters);
        return 0;
    }

    int count = 0;
    for (Posting* p0 = lists[0]; p0; p0 = p0->next) {
        int doc = p0->docID;
        int ok = 1;
        double score = (double)p0->frequency;

        for (int i = 1; i < term_count; i++) {
            Posting* cur = iters[i];
            while (cur && cur->docID < doc) cur = cur->next;
            iters[i] = cur;
            if (!cur || cur->docID != doc) {
                ok = 0;
                break;
            }
            score += (double)cur->frequency;
        }

        if (ok) {
            if (count >= cap) {
                cap *= 2;
                SearchResult* grown = (SearchResult*)realloc(results, (size_t)cap * sizeof(SearchResult));
                if (!grown) {
                    free(results);
                    free(lists);
                    free(iters);
                    return 0;
                }
                results = grown;
            }
            results[count].docID = doc;
            results[count].score = score;
            count++;
        }
    }

    free(lists);
    free(iters);
    *out_results = results;
    *out_count = count;
    return 1;
}

int search_query(const char* query, SearchResult** out_results, int* out_count) {
    if (!out_results || !out_count) return 0;
    *out_results = NULL;
    *out_count = 0;
    if (!query) return 1;

    char** terms = NULL;
    int count = 0;
    if (!tokenize_text_dynamic(query, &terms, &count)) return 0;
    int ok = search_and_terms(terms, count, out_results, out_count);
    tokenize_free_tokens(terms, count);
    return ok;
}

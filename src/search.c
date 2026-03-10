#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "search.h"
#include "index.h"
#include "query_parser.h"
#include "ranking.h"
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

    const RankingMode mode = ranking_get_mode();
    const int n_docs = get_document_count();
    double* idf = NULL;
    if (mode == RANK_TFIDF) {
        idf = (double*)malloc((size_t)term_count * sizeof(double));
        if (!idf) {
            free(lists);
            free(iters);
            return 0;
        }
        for (int i = 0; i < term_count; i++) {
            int df = get_document_frequency(terms[i]);
            if (df <= 0 || n_docs <= 0) idf[i] = 0.0;
            else idf[i] = log((double)n_docs / (double)df);
        }
    }

    for (int i = 0; i < term_count; i++) {
        lists[i] = get_postings(terms[i]);
        iters[i] = lists[i];
        if (!lists[i]) {
            free(lists);
            free(iters);
            free(idf);
            return 1;
        }
    }

    int cap = 32;
    SearchResult* results = (SearchResult*)malloc((size_t)cap * sizeof(SearchResult));
    if (!results) {
        free(lists);
        free(iters);
        free(idf);
        return 0;
    }

    int count = 0;
    for (Posting* p0 = lists[0]; p0; p0 = p0->next) {
        int doc = p0->docID;
        int ok = 1;
        double score = (mode == RANK_TFIDF && idf) ? ((double)p0->frequency * idf[0]) : (double)p0->frequency;

        for (int i = 1; i < term_count; i++) {
            Posting* cur = iters[i];
            while (cur && cur->docID < doc) cur = cur->next;
            iters[i] = cur;
            if (!cur || cur->docID != doc) {
                ok = 0;
                break;
            }
            if (mode == RANK_TFIDF && idf) score += (double)cur->frequency * idf[i];
            else score += (double)cur->frequency;
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
    free(idf);
    if (count > 0) rank_results(results, count);
    *out_results = results;
    *out_count = count;
    return 1;
}

static int search_or_terms(char** terms, int term_count, SearchResult** out_results, int* out_count) {
    *out_results = NULL;
    *out_count = 0;
    if (term_count <= 0) return 1;

    int n_docs = get_document_count();
    if (n_docs <= 0) return 1;
    const RankingMode mode = ranking_get_mode();

    double* scores = (double*)calloc((size_t)(n_docs + 1), sizeof(double));
    if (!scores) return 0;

    double* idf = NULL;
    if (mode == RANK_TFIDF) {
        idf = (double*)malloc((size_t)term_count * sizeof(double));
        if (!idf) {
            free(scores);
            return 0;
        }
        for (int i = 0; i < term_count; i++) {
            int df = get_document_frequency(terms[i]);
            if (df <= 0 || n_docs <= 0) idf[i] = 0.0;
            else idf[i] = log((double)n_docs / (double)df);
        }
    }

    for (int i = 0; i < term_count; i++) {
        Posting* p = get_postings(terms[i]);
        while (p) {
            if (p->docID >= 1 && p->docID <= n_docs) {
                if (mode == RANK_TFIDF && idf) scores[p->docID] += (double)p->frequency * idf[i];
                else scores[p->docID] += (double)p->frequency;
            }
            p = p->next;
        }
    }

    int cap = 32;
    SearchResult* results = (SearchResult*)malloc((size_t)cap * sizeof(SearchResult));
    if (!results) {
        free(scores);
        free(idf);
        return 0;
    }

    int count = 0;
    for (int doc = 1; doc <= n_docs; doc++) {
        if (scores[doc] > 0.0) {
            if (count >= cap) {
                cap *= 2;
                SearchResult* grown = (SearchResult*)realloc(results, (size_t)cap * sizeof(SearchResult));
                if (!grown) {
                    free(results);
                    free(scores);
                    free(idf);
                    return 0;
                }
                results = grown;
            }
            results[count].docID = doc;
            results[count].score = scores[doc];
            count++;
        }
    }

    free(scores);
    free(idf);
    if (count > 0) rank_results(results, count);
    *out_results = results;
    *out_count = count;
    return 1;
}

static int is_or_token(const char* term) {
    return term && strcmp(term, "or") == 0;
}

int search_query(const char* query, SearchResult** out_results, int* out_count) {
    if (!out_results || !out_count) return 0;
    *out_results = NULL;
    *out_count = 0;
    if (!query) return 1;

    Query parsed;
    int has_or = 0;
    if (!query_parse(query, &parsed)) return 0;
    for (int i = 0; i < parsed.count; i++) {
        if (parsed.tokens[i].type == QUERY_TOKEN_OR) {
            has_or = 1;
            break;
        }
    }
    query_free(&parsed);

    char** raw_terms = NULL;
    int raw_count = 0;
    if (!tokenize_text_dynamic(query, &raw_terms, &raw_count)) return 0;

    if (has_or) {
        int kept = 0;
        for (int i = 0; i < raw_count; i++) if (!is_or_token(raw_terms[i])) kept++;
        if (kept == 0) {
            tokenize_free_tokens(raw_terms, raw_count);
            return 1;
        }

        char** terms = (char**)malloc((size_t)kept * sizeof(char*));
        if (!terms) {
            tokenize_free_tokens(raw_terms, raw_count);
            return 0;
        }
        int n = 0;
        for (int i = 0; i < raw_count; i++) {
            if (is_or_token(raw_terms[i])) continue;
            terms[n++] = raw_terms[i];
            raw_terms[i] = NULL;
        }
        for (int i = 0; i < raw_count; i++) free(raw_terms[i]);
        free(raw_terms);

        int ok = search_or_terms(terms, n, out_results, out_count);
        tokenize_free_tokens(terms, n);
        return ok;
    }

    {
        int ok = search_and_terms(raw_terms, raw_count, out_results, out_count);
        tokenize_free_tokens(raw_terms, raw_count);
        return ok;
    }
}

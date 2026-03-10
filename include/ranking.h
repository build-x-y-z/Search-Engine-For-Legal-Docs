#ifndef RANKING_H
#define RANKING_H

#include "structures.h"

typedef enum {
    RANK_TF = 0,
    RANK_TFIDF = 1
} RankingMode;

void ranking_set_mode(RankingMode mode);
RankingMode ranking_get_mode(void);

void rank_results(SearchResult* results, int count);
void rank_results_mode(SearchResult* results, int count, RankingMode mode);

#endif

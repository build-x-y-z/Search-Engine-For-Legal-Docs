#include "ranking.h"

static void swap(SearchResult* a, SearchResult* b) {
    SearchResult t = *a;
    *a = *b;
    *b = t;
}

static void qsort_results(SearchResult* arr, int left, int right) {
    int i = left;
    int j = right;
    SearchResult pivot = arr[left + (right - left) / 2];

    while (i <= j) {
        while (arr[i].score > pivot.score) i++;
        while (arr[j].score < pivot.score) j--;
        if (i <= j) {
            swap(&arr[i], &arr[j]);
            i++;
            j--;
        }
    }
    if (left < j) qsort_results(arr, left, j);
    if (i < right) qsort_results(arr, i, right);
}

void rank_results(SearchResult* results, int count) {
    if (!results || count <= 1) return;
    qsort_results(results, 0, count - 1);
}

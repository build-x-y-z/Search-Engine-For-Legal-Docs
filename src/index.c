#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"

#define HASH_SIZE 4096

static WordEntry *g_table[HASH_SIZE];

static unsigned long hash_word(const char *s)
{
    unsigned long h = 5381UL;
    int c;
    while ((c = (unsigned char)*s++) != 0)
        h = ((h << 5) + h) + (unsigned long)c;
    return h;
}

static WordEntry *find_entry(const char *word, unsigned long *out_bucket)
{
    unsigned long b = hash_word(word) % (unsigned long)HASH_SIZE;
    if (out_bucket)
        *out_bucket = b;
    WordEntry *cur = g_table[b];
    while (cur)
    {
        if (strcmp(cur->word, word) == 0)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

void index_init(void)
{
    for (int i = 0; i < HASH_SIZE; i++)
        g_table[i] = NULL;
}

static void free_positions(PositionNode *pos)
{
    while (pos)
    {
        PositionNode *next = pos->next;
        free(pos);
        pos = next;
    }
}

static void free_postings(Posting *p)
{
    while (p)
    {
        Posting *next = p->next;
        free_positions(p->positions);
        free(p);
        p = next;
    }
}

void index_free(void)
{
    for (int i = 0; i < HASH_SIZE; i++)
    {
        WordEntry *e = g_table[i];
        while (e)
        {
            WordEntry *next = e->next;
            free_postings(e->postingList);
            free(e);
            e = next;
        }
        g_table[i] = NULL;
    }
}

static Posting *find_posting(Posting *head, int docID, Posting **out_prev)
{
    Posting *prev = NULL;
    Posting *cur = head;
    while (cur && cur->docID < docID)
    {
        prev = cur;
        cur = cur->next;
    }
    if (out_prev)
        *out_prev = prev;
    if (cur && cur->docID == docID)
        return cur;
    return NULL;
}

void insert_word(char *word, int docID)
{
    insert_term(word, docID, -1);
}

void insert_term(const char *word, int docID, int position)
{
    if (!word || !word[0] || docID <= 0)
        return;

    unsigned long bucket = 0;
    WordEntry *e = find_entry(word, &bucket);
    if (!e)
    {
        e = (WordEntry *)malloc(sizeof(WordEntry));
        if (!e)
            return;
        memset(e, 0, sizeof(WordEntry));
        strncpy(e->word, word, WORD_MAX_LEN - 1);
        e->next = g_table[bucket];
        g_table[bucket] = e;
    }

    Posting *prev = NULL;
    Posting *p = find_posting(e->postingList, docID, &prev);
    if (p)
    {
        p->frequency += 1;
        if (position >= 0)
        {
            PositionNode *pos = (PositionNode *)malloc(sizeof(PositionNode));
            if (!pos)
                return;
            pos->position = position;
            pos->next = p->positions;
            p->positions = pos;
        }
        return;
    }

    Posting *node = (Posting *)malloc(sizeof(Posting));
    if (!node)
        return;
    node->docID = docID;
    node->frequency = 1;
    node->positions = NULL;
    if (position >= 0)
    {
        PositionNode *pos = (PositionNode *)malloc(sizeof(PositionNode));
        if (!pos)
        {
            free(node);
            return;
        }
        pos->position = position;
        pos->next = NULL;
        node->positions = pos;
    }
    e->documentFrequency += 1;
    if (!prev)
    {
        node->next = e->postingList;
        e->postingList = node;
    }
    else
    {
        node->next = prev->next;
        prev->next = node;
    }
}

Posting *get_postings(char *word)
{
    if (!word || !word[0])
        return NULL;
    WordEntry *cur = find_entry(word, NULL);
    return cur ? cur->postingList : NULL;
}

int get_document_frequency(const char *word)
{
    if (!word || !word[0])
        return 0;
    WordEntry *cur = find_entry(word, NULL);
    return cur ? cur->documentFrequency : 0;
}

int get_vocabulary_size(void)
{
    int total = 0;
    for (int i = 0; i < HASH_SIZE; i++)
    {
        for (WordEntry *e = g_table[i]; e; e = e->next)
            total++;
    }
    return total;
}

void debug_print_index()
{
    printf("\n===== INVERTED INDEX TABLE =====\n");

    for (int i = 0; i < HASH_SIZE; i++)
    {
        if (g_table[i] != NULL)
        {
            printf("\nBucket %d:\n", i);

            WordEntry *e = g_table[i];
            while (e)
            {
                printf("  Word: %s | DF: %d | Hash: %lu",
                       e->word,
                       e->documentFrequency,
                       hash_word(e->word) % HASH_SIZE);

                // show linkage clearly
                if (e->next)
                {
                    printf("  ---> next");
                }

                printf("\n");

                Posting *p = e->postingList;
                while (p)
                {
                    printf("    -> DocID: %d | Freq: %d\n", p->docID, p->frequency);

                    PositionNode *pos = p->positions;
                    if (pos)
                    {
                        printf("       Positions: ");
                        while (pos)
                        {
                            printf("%d ", pos->position);
                            pos = pos->next;
                        }
                        printf("\n");
                    }

                    p = p->next;
                }

                e = e->next;
            }
        }
    }
}
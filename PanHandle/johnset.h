#include "uthash.h"

typedef struct PAN {
    unsigned long hash;
    UT_hash_handle hh;
} PAN;

PAN* johnset_initialize() {
    PAN *pans = NULL;
    return pans;
}

// djb2 hash function
// http://www.cse.yorku.ca/~oz/hash.html
unsigned long djb2(char *key, size_t len)
{
    unsigned long hash = 5381;
    int c;
    for (int i = 0; i < len; i++) {
        c = *key++;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

void johnset_add(PAN **pans, char *str) {
    PAN *s = (PAN*) malloc(sizeof(PAN));
    s->hash = djb2(str, 20);
    HASH_ADD(hh, *pans, hash, sizeof(unsigned long), s);
}

int johnset_exists(PAN *pans, unsigned long h) {
    PAN *s;
    HASH_FIND(hh, pans, &h, sizeof(unsigned long), s);
    return s ? 1 : 0;
}

void johnset_free(PAN *pans) {
    free(pans);
}
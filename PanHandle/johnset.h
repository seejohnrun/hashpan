#include "uthash.h"

/**
 This is a Set implementation on top of UTHash.  It allows us to easily
 maintain which PANs were in the original set and look them up in constant
 time
 */

typedef struct PAN {
    unsigned long hash;
    UT_hash_handle hh;
} PAN;

// create a new set
PAN* johnset_initialize() {
    PAN *pans = NULL;
    return pans;
}

// the hash function to use for our strings (char*)
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

// add a new member to the set
void johnset_add(PAN **pans, char *str) {
    PAN *s = (PAN*) malloc(sizeof(PAN));
    s->hash = djb2(str, 20);
    HASH_ADD(hh, *pans, hash, sizeof(unsigned long), s);
}

// check if a member exists
int johnset_exists(PAN *pans, unsigned long h) {
    PAN *s;
    HASH_FIND(hh, pans, &h, sizeof(unsigned long), s);
    return s ? 1 : 0;
}

// free up the set, we're done here
void johnset_free(PAN *pans) {
    free(pans);
}
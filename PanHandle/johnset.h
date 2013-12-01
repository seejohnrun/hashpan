#include "uthash.h"

typedef struct PAN {
    uint32_t hash;
    UT_hash_handle hh;
} PAN;

PAN* johnset_initialize() {
    PAN *pans = NULL;
    return pans;
}

unsigned int jenkins_one_at_a_time_hash(char *key, size_t len)
{
    unsigned int hash, i;
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

void johnset_add(PAN **pans, char *str) {
    PAN *s = (PAN*) malloc(sizeof(PAN));
    s->hash = jenkins_one_at_a_time_hash(str, 20);
    HASH_ADD_INT(*pans, hash, s);
}

int johnset_exists(PAN *pans, char *str) {
    PAN *s;
    uint32_t hash = jenkins_one_at_a_time_hash(str, 20);
    HASH_FIND_INT(pans, &hash, s);
    return s ? 1 : 0;
}

void johnset_free(PAN *pans) {
    free(pans);
}
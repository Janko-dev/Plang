#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <string.h>
#include <stdlib.h>

typedef struct nlist Hashlist;
struct nlist{
    struct nlist* next;
    char* key;
    int val;
};

#define HASH_SIZE 100
extern Hashlist* hashtab[HASH_SIZE];

Hashlist* put(char* key, int val);
Hashlist* lookup(char* s);

#endif //_HASHTABLE_H


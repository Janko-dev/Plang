#include "hashtable.h"

Hashlist* hashtab[HASH_SIZE];

unsigned int hash(char* s){
    unsigned int hashval;
    for (hashval = 0; *s != '\0'; s++){
        hashval = *s + 31 * hashval;
    }

    return hashval % HASH_SIZE;
}

char* strdup(char* s){
    char* p = (char*)malloc(strlen(s) + 1);
    if (p != NULL) strcpy(p, s);
    return p;
}

Hashlist* lookup(char* s){
    Hashlist* h;
    for (h = hashtab[hash(s)]; h != NULL; h = h->next){
        if (strcmp(s, h->key) == 0){
            return h;
        }
    }
    return NULL;
}

Hashlist* put(char* key, int val){
    Hashlist* h;
    unsigned int hashval;
    if ((h = lookup(key)) == NULL){
        h = (Hashlist*)malloc(sizeof(*h));
        if (h == NULL || (h->key = strdup(key)) == NULL) return NULL;
        hashval = hash(key);
        h->next = hashtab[hashval];
        hashtab[hashval] = h;
    }
    h->val = val;
    return h;
}

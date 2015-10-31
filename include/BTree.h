#ifndef _BTREE_H
#define _BTREE_H
#include <string.h>
#include "alloc.h"
#include "common.h"
//#include "xerror.h"

typedef int (*CMP)(const void *a, const void *b);
int cmpInt(const void *a, const void *b);
int cmpFloat(const void *a, const void *b);
int cmpStr(const void *a, const void *b);
//...

typedef struct btree_item btree_item;
typedef struct btree_node btree_node;
typedef struct BTree BTree;
typedef struct BTreeEnum BTreeEnum;

#define KEY_LENGTH	126
struct btree_item {
	handle_t child;
	uint8_t key[KEY_LENGTH];
} __attribute__((packed));

#define NODE_SIZE ((4096 - 2) / sizeof(struct btree_item))
#define M (NODE_SIZE-1)
struct btree_node {
    uint8_t tag;
	uint8_t size;
	struct btree_item items[NODE_SIZE];
    //handle_t prev, next;
} __attribute__((packed));

struct BTree{
    ALLOC *store;
    handle_t root;
    handle_t iroot;
    CMP collate;
    //uint64_t serial;
};

struct BTreeEnum {
    ALLOC *store;
    handle_t id;
    int index;
	uint8_t key[KEY_LENGTH];
    handle_t value;
    //uint64_t serial;
};

//using the follow function, you should malloc the struct BTree and BTreeEnum firstly.
extern handle_t CreateBTree(BTree *bt, ALLOC *store, CMP collate);
//OpenBTree by the handle_t got from CreateBTree.
extern void OpenBTree(BTree *bt, ALLOC *store, CMP collate, handle_t handle);
extern void ClearBTree(BTree *bt);
extern void SetKey(BTree *bt, void *key, handle_t value);
extern handle_t GetKey(BTree *bt, void *key);
extern void DeleteKey(BTree *bt, void *key);


extern int IsValid(BTreeEnum *bte);
//return value = 1, bte->key = key; value=0, bte->key>key or not found;
extern int EnumLower_bound(BTreeEnum *bte, BTree *bt, const void *key); //>=key
extern void EnumBegin(BTreeEnum *bte, BTree *bt);
extern void EnumEnd(BTreeEnum *bte, BTree *bt);
//return value = 1, equals; = 0, dong't equals
extern int IsEqual(BTreeEnum *x, BTreeEnum *y);
extern void MoveNext(BTreeEnum *bte);
extern const uint8_t *BTKey(BTreeEnum *bte);
extern const handle_t BTValue(BTreeEnum *bte);
#endif


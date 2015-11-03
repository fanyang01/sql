//imxian(imkzy@foxmail.com)
//This version supports non-unique and variable-length key.
#ifndef _BTREE_H
#define _BTREE_H
#include "alloc.h"

typedef int (*CMP)(const void *a, const void *b);
extern int cmpInt(const void *a, const void *b);
extern int cmpFloat(const void *a, const void *b);
extern int cmpStr(const void *a, const void *b);

typedef struct btree_item btree_item;
typedef struct btree_node btree_node;
typedef struct BTree BTree;
typedef struct BTreeEnum BTreeEnum;

#define KEY_LENGTH (256+8)
#define BLOCK_SIZE 4096

/*
 *struct btree_node 
 * m = TABLE_SIZE
 * +--+--+--+---+----+----+----+----------+
 * |p0|k0|p1|...|Km-2|Pm-1|Km-1|Pm|km|Pm+1|
 * +--+--+--+---+----+----+----+--+--+----+
 * if it's a leaf node, pm+1 -> next leaf node;
 * Pm/Km is used for insertion
 */

/*
struct btree_item {
    handle_t child;
    uint8_t key[KEY_LENGTH];
} __attribute__((packed));

#define NODE_SIZE ((4096 - 3) / sizeof(struct btree_item))
#define TABLE_SIZE (NODE_SIZE-2) 
struct btree_node {
    uint8_t isLeaf; //isLeaf = 1, leaf node; isLeaf = 0, nonleaf node;
    uint16_t size;  //the current sum of key stored
    struct btree_item items[NODE_SIZE]; //store pointer and key
} __attribute__((packed));
*/

struct btree_node {
    uint8_t items[BLOCK_SIZE-5]; //store pointer and key
    uint8_t isLeaf; //isLeaf = 1, leaf node; isLeaf = 0, nonleaf node;
    uint16_t size;  //the current sum of key stored
    uint16_t keyLength;//the length of the key
} __attribute__((packed));

struct BTree{
    ALLOC *store; //fileno
    handle_t root; //pointer point to the root
    handle_t iroot; //real root address
    CMP collate; //compare function, use for comparing keys
    uint8_t isUnique; //allow same keys or not
    uint16_t keyLength;//the length of the key
    uint16_t M;//branch number of Btree
};

struct BTreeEnum {
    ALLOC *store; //fileno
    handle_t id; //which node
    int index; // which key in this node
    uint8_t key[KEY_LENGTH]; //key
    handle_t value; //value
    uint8_t isUnique; //allow same keys or not
    uint16_t keyLength;//the length of the key
    uint16_t M;//branch number of Btree
};

//to use the following function, you should malloc the space for struct BTree and BTreeEnum firstly!
//Create a Btree
extern handle_t CreateBTree(BTree *bt, ALLOC *store, uint8_t isUnique, uint16_t keyLength, CMP collate);
//OpenBTree by the handle_t got from CreateBTree.
extern void OpenBTree(BTree *bt, ALLOC *store, uint8_t isUnique, uint16_t keyLength, CMP collate, handle_t handle);
//clear the whole Btree
extern void ClearBTree(BTree *bt);
//set the key/value
extern void SetKey(BTree *bt, const void *key, handle_t value); 
//get the value by key, shouldn't be used in non-unique btree.
extern handle_t GetKey(BTree *bt, const void *key);
//delete the key/value, (for unique Btree, value will be ignored)
extern void DeleteKey(BTree *bt, const void *key, handle_t value);

//all the function below may return an invalid enumerator,
//you should check it using IsValid() or compare with EnumEnd() using IsEqual()!
extern int IsValid(BTreeEnum *bte);
//return the first enumerator whose key >= input key
extern void EnumLower_bound(BTreeEnum *bte, BTree *bt, const void *key); //>=key
//return the first enumerator whose key > input key
extern void EnumUpper_bound(BTreeEnum *bte, BTree *bt, const void *key); //>key
//return the smallest key/pointer
extern void EnumBegin(BTreeEnum *bte, BTree *bt);
//EnumEnd() just returns the invalid enumerator for IsEqual()
extern void EnumEnd(BTreeEnum *bte, BTree *bt);
//return value = 1, equals; value = 0, don't equals
extern int IsEqual(BTreeEnum *x, BTreeEnum *y);
//take the next key/pointer
extern void MoveNext(BTreeEnum *bte);
//return the key, it isn't just bte->key due to non-unique key.
extern const uint8_t *BTKey(BTreeEnum *bte);
//return the value, it is just bte->value
extern const handle_t BTValue(BTreeEnum *bte);

#endif


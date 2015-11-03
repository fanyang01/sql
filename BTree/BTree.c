//imxian(imkzy@foxmail.com)
//This version supports non-unique and variable-length key.
#include "BTree.h"
#include <string.h>
#include <time.h>
#include <assert.h>
//#include <stdio.h>

//malloc btree space in file and set the root address 0
static handle_t newBTree(ALLOC *store);
//get the root address
static handle_t getRoot(BTree *bt);
//set the new root address
static void flushRoot(BTree *bt, handle_t ph);
//free the node and its children
static void clearNode(BTree *bt, handle_t ph);
//alloc a node
static handle_t allocNode(BTree *bt);
//read the node information to a buffer
static void *getNode(BTree *bt, handle_t ph);
//free a buffer
static void putNode(BTree *bt, void *p);
//write a node
static void flushNode(BTree *bt, handle_t ph, btree_node *p);
//free a node
static void freeNode(BTree *bt, handle_t ph);
//find the key in node p, return its index
static int findKey(BTree *bt, btree_node *p, const void *key, int *index);
//split a node
static handle_t splitNode(BTree *bt, btree_node *p, void *key);
//insert a key in a node
static void insertKey(BTree *bt, handle_t ph, void *key, handle_t value);
//merge a node with its sibling, or replace them
static int isMerge(BTree *bt, btree_node *p, btree_node *child, int i, int isPred);
//erase a key in a node
static void eraseKey(BTree *bt, handle_t ph, const void *key);
//shift left a node, delete its first k/p
static void shiftLeft(btree_node *p);
//shift right a node for a new first k/p
static void shiftRight(btree_node *p);
//delete Ki/Pi+1 in the node
static void moveLeft(btree_node *p, int i);
//compare handle_t
static int cmpHandle(const void *a, const void *b);
//compare key
static int keyCmp(BTree *bt, const void *a, const void *b);
//merge pointer and key as a new key if needed
static void convertKey(BTree *bt, const void *key, handle_t value, uint8_t *Key);
//get the key from the node by index
static uint8_t *pKey(btree_node *p, int index);
//get the child from the node by index
static handle_t *pChild(btree_node *p, int index);

handle_t CreateBTree(BTree *bt, ALLOC *store, uint8_t isUnique, uint16_t keyLength, CMP collate) {
    bt->store = store;
    bt->isUnique = isUnique;
    bt->collate = collate;
    bt->root = newBTree(store);
    bt->iroot = 0;
    if (bt->isUnique) bt->keyLength = keyLength + 1;
    else bt->keyLength = sizeof(handle_t) + keyLength + 1;
    bt->M = (BLOCK_SIZE - 5) / (sizeof(handle_t) + bt->keyLength) - 2;
    return bt->root;
}

static uint8_t zeros[sizeof(handle_t)];
handle_t newBTree(ALLOC *store) {
    handle_t t = alloc_blk(store, zeros, sizeof(handle_t));
    assert(t > 0);
    return t;
}

void OpenBTree(BTree *bt, ALLOC *store, uint8_t isUnique, uint16_t keyLength, CMP collate, handle_t handle) {
    bt->store = store;
    bt->isUnique = isUnique;
    bt->collate = collate;
    bt->root = handle;
    bt->iroot = getRoot(bt);
    if (bt->isUnique) bt->keyLength = keyLength + 1;
    else bt->keyLength = sizeof(handle_t) + keyLength + 1;
    bt->M = (BLOCK_SIZE - 5) / (sizeof(handle_t) + bt->keyLength) - 2;
}

handle_t getRoot(BTree *bt) {
    size_t len = 0;
    void *r = read_blk(bt->store, bt->root, NULL, &len);
    assert(r != NULL);
    handle_t iroot = *(handle_t*)r;
    buf_put(bt->store, r);
    return iroot;
}

void flushRoot(BTree *bt, handle_t ph) {
    realloc_blk(bt->store, bt->root, &ph, sizeof(handle_t));
    bt->iroot = ph;
}

void ClearBTree(BTree *bt) {
    if(bt == NULL) return;
    if(bt->iroot == 0) return;
    clearNode(bt, bt->iroot);
    flushRoot(bt, 0);
    //printf("%ld\n", bt->iroot);
}

void clearNode(BTree *bt, handle_t ph) {
    int i;
    btree_node *p = getNode(bt, ph);
    if (!p->isLeaf) {
        for (i = 0; i <= p->size; i++) {
            clearNode(bt, *pChild(p, i));
        }
    }
    putNode(bt, p);
    freeNode(bt, ph);
}


handle_t allocNode(BTree *bt) {
    /*
    void *a = calloc(1, sizeof(btree_node));
    printf("alloc %p\n", a);
    return a;
    */
    btree_node btn;
    handle_t ph = alloc_blk(bt->store, &btn, sizeof(btree_node));
//    printf("alloc %ld\n", ph);
    assert(ph > 0);
    return ph;
}

void *getNode(BTree *bt, handle_t ph) {
    /*
    void *a = calloc(1, sizeof(btree_node));
    printf("alloc %p\n", a);
    memcpy(a, (void*)ph, sizeof(btree_node));
    return a;
    */
    size_t len = 0;
    void *r = read_blk(bt->store, ph, NULL, &len);
    //printf("alloc %p\n", r);
    assert(r != NULL);
    return r;
}

void putNode(BTree *bt, void *p) {
    /*
    printf("free %p\n", p);
    free(p);
    */
    buf_put(bt->store, p);
//    printf("free %p\n", p);
}

void flushNode(BTree *bt, handle_t ph, btree_node *p) {
    /*
    memcpy((void*)ph, p, sizeof(btree_node));
    putNode(bt,p);
    */
    realloc_blk(bt->store, ph, p, sizeof(btree_node));
    putNode(bt, p);
}

void freeNode(BTree *bt, handle_t ph) {
    /*
    printf("Free %ld\n", ph);
    free(ph);
    */
    //printf("Free %ld\n", ph);
    dealloc_blk(bt->store, ph);
}

handle_t splitNode(BTree *bt, btree_node *p, void *key) {
    handle_t ph = allocNode(bt);
    btree_node *new_node = getNode(bt, ph);
    new_node->keyLength = bt->keyLength;
    new_node->isLeaf = p->isLeaf;
    if (p->isLeaf) {
        memcpy(key, pKey(p, (bt->M+1)/2), bt->keyLength);
        p->size = (bt->M+1)/2;
        new_node->size = bt->M - p->size;
        memcpy(pChild(new_node, 0), pChild(p, (bt->M+1)/2), (new_node->size+1)*(sizeof(handle_t)+new_node->keyLength));
        *pChild(new_node, bt->M+1) = *pChild(p, bt->M+1);
        *pChild(p, bt->M+1) = ph;
        *pChild(p, p->size) = 0;
        flushNode(bt, ph, new_node);
        return ph;
    }
    else {
        memcpy(key, pKey(p, (bt->M-1)/2), bt->keyLength);
        p->size = (bt->M-1)/2;
        new_node->size = bt->M - p->size - 1;
        memcpy(pChild(new_node, 0), pChild(p, (bt->M-1)/2+1), (new_node->size+1)*(sizeof(handle_t)+new_node->keyLength));
        flushNode(bt, ph, new_node);
        return ph;
    }
}

void insertKey(BTree *bt, handle_t ph, void *key, handle_t value) {
    btree_node *p = getNode(bt, ph);
    int i;
    int ok = findKey(bt, p, key, &i);
    if (ok) i++;
    handle_t left_child = *pChild(p, i), right_child = 0;
    if (!p->isLeaf) {
        insertKey(bt, left_child, key, value);
        btree_node *child = getNode(bt, left_child); 
        if (child->size < bt->M) {
            putNode(bt, p);
            putNode(bt, child);
            return;
        }
        right_child = splitNode(bt, child, key);
        flushNode(bt, left_child, child);
    } else {
        //printf("find %d %d\n", i, ok);
        if (ok) {
            *pChild(p, i-1) = value;
            flushNode(bt, ph, p);
            return;
        }
        left_child = value;
        right_child = *pChild(p, i);
    }
    p->size++;
    memmove(pChild(p, i + 1), pChild(p, i), (p->size-i)*(sizeof(handle_t)+p->keyLength));
    memcpy(pKey(p, i), key, bt->keyLength);
    *pChild(p, i) = left_child;
    *pChild(p, i + 1) = right_child;
    flushNode(bt, ph, p);
}

void SetKey(BTree *bt, const void *key, handle_t value) {
    if (bt == NULL) return;
    uint8_t Key[bt->keyLength];
    convertKey(bt, key, value, Key);
    //memcpy(Key, key, bt->keyLength);
    handle_t left_child, right_child;
    if (bt->iroot != 0) {
        insertKey(bt, bt->iroot, Key, value);
        btree_node *p = getNode(bt, bt->iroot);
        //pout(p);
        if(p->size < bt->M) {
            putNode(bt, p);
            return;
        }
        left_child = bt->iroot;
        right_child = splitNode(bt, p, Key);
        flushNode(bt, bt->iroot, p);
    } else {
        left_child = value;
        right_child = 0;
    }
    handle_t ph = allocNode(bt);
    btree_node *new_node = getNode(bt, ph);
    new_node->keyLength = bt->keyLength;
    if(bt->iroot == 0) {
        new_node->isLeaf = 1;
        *pChild(new_node, bt->M+1) = 0;
    }
    else new_node->isLeaf = 0;
    new_node->size = 1;
    memcpy(pKey(new_node, 0), Key, bt->keyLength);
    *pChild(new_node, 0) = left_child;
    *pChild(new_node, 1) = right_child;
    //pout(new_node);
    flushNode(bt, ph, new_node);
    flushRoot(bt, ph);
}


void shiftRight(btree_node *p) {
    memmove(pChild(p, 1), pChild(p, 0), (p->size+1)*(sizeof(handle_t)+p->keyLength));
}

void shiftLeft(btree_node *p) {
    memmove(pChild(p, 0), pChild(p, 1), p->size*(sizeof(handle_t)+p->keyLength));
}

int isMerge(BTree *bt, btree_node *p, btree_node *child, int i, int isPred) {
    //printf("isMerge %d %d\n", i, isPred);
    btree_node *sibling;
    if (isPred) sibling = getNode(bt, *pChild(p, i+1));
    else sibling = getNode(bt, *pChild(p, i));
    if ( (child->isLeaf && sibling->size+child->size < bt->M) ||
            (!child->isLeaf && sibling->size+child->size < bt->M-1) ) {
        if (!isPred) {
            btree_node *tmp = child;
            child = sibling;
            sibling = tmp;
        }
        if (child->isLeaf) {
            memcpy(pChild(child, child->size), pChild(sibling, 0), (sibling->size+1)*(sizeof(handle_t)+sibling->keyLength));
            *pChild(child, bt->M+1) = *pChild(sibling, bt->M+1);
            child->size += sibling->size;
        }
        else {
            memcpy(pKey(child, child->size), pKey(p, i), bt->keyLength);
            memcpy(pChild(child, child->size+1), pChild(sibling, 0), (sibling->size+1)*(sizeof(handle_t)+sibling->keyLength));
            child->size += sibling->size + 1;
        }
        //pout(child);
        flushNode(bt, *pChild(p, i), child);
        putNode(bt, sibling);
        freeNode(bt, *pChild(p, i+1));
        return 1;
    }
    if (!isPred) {
        shiftRight(child);
        if (!child->isLeaf) {
            memcpy(pKey(child, 0), pKey(p, i), bt->keyLength);
            *pChild(child, 0) = *pChild(sibling, sibling->size);
            memcpy(pKey(p, i), pKey(sibling, sibling->size-1), bt->keyLength);
        }
        else {
            memcpy(pKey(child, 0), pKey(sibling, sibling->size-1), bt->keyLength);
            *pChild(child, 0) = *pChild(sibling, sibling->size-1);
            *pChild(sibling, sibling->size-1) = 0;
            memcpy(pKey(p, i), pKey(child, 0), bt->keyLength);
        }
        sibling->size--;
        child->size++;
        flushNode(bt, *pChild(p, i), sibling);
        flushNode(bt, *pChild(p, i+1), child);
    }
    else {
        if (!child->isLeaf) {
            memcpy(pKey(child, child->size), pKey(p, i), bt->keyLength);
            *pChild(child, child->size+1) = *pChild(sibling, 0);
            memcpy(pKey(p, i), pKey(sibling, 0), bt->keyLength);
        }
        else {
            memcpy(pKey(child, child->size), pKey(sibling, 0), bt->keyLength);
            *pChild(child, child->size) = *pChild(sibling, 0);
            memcpy(pKey(p, i), pKey(sibling, 1), bt->keyLength);
        }
        shiftLeft(sibling);
        sibling->size--;
        child->size++;
        flushNode(bt, *pChild(p, i), child);
        flushNode(bt, *pChild(p, i+1), sibling);
    }
    return 0;
}

void moveLeft(btree_node *p, int i) {
    handle_t tmp = *pChild(p, i);
    memmove(pChild(p, i), pChild(p, i + 1), (p->size-i)*(sizeof(handle_t)+p->keyLength));
    p->size--;
    *pChild(p, i) = tmp;
}

void eraseKey(BTree *bt, handle_t ph, const void *key) {
    btree_node *p = getNode(bt, ph);
    //printf("erase %d ", ph);
    //pout(p);
    int i;
    int ok = findKey(bt, p, key, &i);
    if (ok) i++;
    handle_t left_child = *pChild(p, i);
    if (!p->isLeaf) {
        eraseKey(bt, left_child, key);
        btree_node *child = getNode(bt, left_child); 
        //pout(child);
        if ( (child->isLeaf && child->size >= bt->M/2) ||
                (!child->isLeaf && child->size >= (bt->M-1)/2) ) {
            putNode(bt, p);
            putNode(bt, child);
            return;
        }
        int isPred;
        if(i > 0) isPred = 0;
        if(i < p->size) {
            if (time(NULL) % 2 || i == 0) isPred = 1;
        }
        if(!isPred) i--;
        //printf("%d %d\n", i, left_child);
        if (!isMerge(bt, p, child, i, isPred)) {
            flushNode(bt, ph, p);
            return;
        }
        /*
        else {
            btree_node *child = getNode(bt, left_child); 
            pout(child);
            putNode(bt, child);
        }
        */

    } else {
        if(!ok) return;
        i--;
       // printf("%d\n", i);
        *pChild(p, i) = *pChild(p, i+1);
    }
    moveLeft(p, i);
    flushNode(bt, ph, p);
    /*
    btree_node *child = getNode(bt, ph); 
    pout(child);
    putNode(bt, child);
    */
}



void DeleteKey(BTree *bt, const void *key, handle_t value) {
    if (bt == NULL) return;
    if (bt->iroot == 0) return;
    uint8_t Key[bt->keyLength];
    convertKey(bt, key, value, Key);
    eraseKey(bt, bt->iroot, Key);
    btree_node *p = getNode(bt, bt->iroot);
    //pout(p);
    //printf("%d\n", p->size);
    if (p->size == 0) {
        handle_t child = *pChild(p, 0);
        freeNode(bt, bt->iroot);
        flushRoot(bt, child);
        //printf("%d\n", child);
    }
    putNode(bt, p);
}

handle_t GetKey(BTree *bt, const void *key) {
    int ok, index;
    if(bt == NULL) return 0;
    if(bt->iroot == 0) return 0; 
    handle_t ph = bt->iroot;
    while (1) {
        btree_node *p = getNode(bt, ph);
        ok = findKey(bt, p, key, &index);
        if (p->isLeaf) {
            if (ok) ph = *pChild(p, index);
            else ph = 0;
            putNode(bt, p);
            break;
        }
        if (ok) index++;
        ph = *pChild(p, index);
        putNode(bt, p);
    }
    return ph;
}

int findKey(BTree *bt, btree_node *p, const void *key, int *index) {
    int l = -1;
    int r = p->size;
    while (l + 1 < r) {
        int mid = (l + r) >> 1;
        int ret = keyCmp(bt, pKey(p, mid), key);
        if (ret < 0) l = mid;
        else r = mid;
    }
    *index = r;
    return keyCmp(bt, pKey(p, *index), key) == 0;
}

void EnumLower_bound(BTreeEnum *bte, BTree *bt, const void *key) {
    EnumEnd(bte, bt);
    if(bt == NULL) return;
    if(bt->iroot == 0) return; 
    uint8_t Key[bt->keyLength];
    convertKey(bt, key, 0, Key);
    handle_t ph = bt->iroot;
    int isHit, index, flag = 0;
    while (1) {
        btree_node *p = getNode(bt, ph);
        isHit = findKey(bt, p, Key, &index);
        if (p->isLeaf) {
            if(index == p->size) {
                flag = 1;
                index--;
            }
            bte->store = bt->store;
            bte->id = ph;
            bte->index = index;
            memcpy(bte->key, pKey(p, index), bte->keyLength);
            bte->value = *pChild(p, index);
            putNode(bt, p);
            if (flag) MoveNext(bte);
            break;
        }
        if (isHit) index++;
        ph = *pChild(p, index);
        putNode(bt, p);
    }
}

void EnumUpper_bound(BTreeEnum *bte, BTree *bt, const void *key) {
    EnumLower_bound(bte, bt, key);
    while(IsValid(bte) && bt->collate(BTKey(bte), key) == 0) MoveNext(bte);
}

void EnumBegin(BTreeEnum *bte, BTree *bt) {
    EnumEnd(bte, bt);
    if(bt == NULL) return;
    if(bt->iroot == 0) return; 
    handle_t ph = bt->iroot;
    while (1) {
        btree_node *p = getNode(bt, ph); 
        if (p->isLeaf) {
            bte->store = bt->store;
            bte->id = ph;
            bte->index = 0;
            memcpy(bte->key, pKey(p, 0), bte->keyLength);
            //printf("%d\n", sizeof(bte->key));
            //printf("%d\n", sizeof(pKey(p, 0)));
            bte->value = *pChild(p, 0);
            putNode(bt, p);
            break;
        }
        ph = *pChild(p, 0);
        putNode(bt, p);
    }
}

void EnumEnd(BTreeEnum *bte, BTree *bt) {
    bte->store = 0;
    bte->id = 0;
    bte->index = 0;
    bte->isUnique = bt->isUnique;
    bte->keyLength = bt->keyLength;
    bte->M = bt->M;
}

void MoveNext(BTreeEnum *bte) {
    if(bte->id == 0) return;
    size_t len = 0;
    btree_node *p = read_blk(bte->store, bte->id, NULL, &len);
    assert(p != NULL);
    if (bte->index < p->size - 1) {
        bte->index++;
        memcpy(bte->key, pKey(p, bte->index), sizeof(bte->key));
        bte->value = *pChild(p, bte->index);
        buf_put(bte->store, p);
    }
    else {
        bte->id = *pChild(p, bte->M+1);
        buf_put(bte->store, p);
        if (bte->id == 0) {
            bte->store = 0;
            bte->index = 0;
            return;
        }
        len = 0;
        p = read_blk(bte->store, bte->id, NULL, &len);
        assert(p != NULL);
        bte->index = 0;
        memcpy(bte->key, pKey(p, bte->index), sizeof(bte->key));
        bte->value = *pChild(p, bte->index);
        buf_put(bte->store, p);
    }
}

int IsValid(BTreeEnum *bte) {
    return bte->id != 0;
}

int IsEqual(BTreeEnum *x, BTreeEnum *y) {
    return x->store == y->store && x->id == y->id && x->index == y->index;
}

const uint8_t *BTKey(BTreeEnum *bte) {
    if (IsValid(bte)) {
        if (bte->isUnique) return bte->key;
        else return bte->key+sizeof(handle_t);
    }
    else return NULL;
}

const handle_t BTValue(BTreeEnum *bte) {
    if (IsValid(bte)) return bte->value;
    else return 0;
}

int cmpInt(const void *a, const void *b) {
    const int *x = a;
    const int *y = b;
    if (*x == *y) return 0;
    else if (*x < *y) return -1;
    else return 1;
}

int cmpFloat(const void *a, const void *b) {
    const float *x = a;
    const float *y = b;
    if (*x == *y) return 0;
    else if (*x < *y) return -1;
    else return 1;
}

int cmpStr(const void *a, const void *b) {
    return strcmp(a, b);
}

int cmpHandle(const void *a, const void *b) {
    const handle_t *x = a;
    const handle_t *y = b;
    if (*x == *y) return 0;
    else if (*x < *y) return -1;
    else return 1;
}

int keyCmp(BTree *bt, const void *a, const void *b) {
    if (bt->isUnique) {
        return bt->collate(a, b);
    }
    else {
        int ret = bt->collate((handle_t *)a + 1, (handle_t *)b + 1);
        if (ret == 0) return cmpHandle(a, b);
        else return ret;
    }
}

void convertKey(BTree *bt, const void *key, handle_t value, uint8_t *Key) {
    if (bt->isUnique) {
        memcpy(Key, key, bt->keyLength);
    }
    else {
        memcpy(Key, &value, sizeof(handle_t));
        memcpy(Key+sizeof(handle_t), key, bt->keyLength-sizeof(handle_t));
    }
}

uint8_t *pKey(btree_node *p, int index) {
    return (uint8_t *)p + (sizeof(handle_t)+p->keyLength)*index+sizeof(handle_t);
}

handle_t *pChild(btree_node *p, int index) {
    return (handle_t*)((uint8_t *)p + (sizeof(handle_t)+p->keyLength)*index);
}



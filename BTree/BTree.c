//imxian(imkzy@foxmail.com)
//This version supports the non-unique key.
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
//merge a node withi its sibling, or replace them
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

/*
void pout(btree_node * child){
        for(int i = 0; i <= TABLE_SIZE+1;i++) {
            printf("%ld %d ", child->items[i].child, *(int*)child->items[i].key);
        }
        puts("");
}
*/

handle_t CreateBTree(BTree *bt, ALLOC *store, uint8_t isUnique, CMP collate) {
    bt->store = store;
    bt->isUnique = isUnique;
    bt->collate = collate;
    bt->root = newBTree(store);
    bt->iroot = 0;
    return bt->root;
}

static uint8_t zeros[sizeof(handle_t)];
handle_t newBTree(ALLOC *store) {
    handle_t t = alloc_blk(store, zeros, sizeof(handle_t));
    assert(t > 0);
    return t;
}

void OpenBTree(BTree *bt, ALLOC *store, uint8_t isUnique, CMP collate, handle_t handle) {
    bt->store = store;
    bt->isUnique = isUnique;
    bt->collate = collate;
    bt->root = handle;
    bt->iroot = getRoot(bt);
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
            clearNode(bt, p->items[i].child);
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
    new_node->isLeaf = p->isLeaf;
    if (p->isLeaf) {
        memcpy(key, p->items[(TABLE_SIZE+1)/2].key, KEY_LENGTH);
        p->size = (TABLE_SIZE+1)/2;
        new_node->size = TABLE_SIZE - p->size;
        memcpy(new_node->items, &p->items[(TABLE_SIZE+1)/2], (new_node->size+1)*sizeof(btree_item));
        new_node->items[TABLE_SIZE+1].child = p->items[TABLE_SIZE+1].child;
        p->items[TABLE_SIZE+1].child = ph;
        p->items[p->size].child = 0;
        flushNode(bt, ph, new_node);
        return ph;
    }
    else {
        memcpy(key, p->items[(TABLE_SIZE-1)/2].key, KEY_LENGTH);
        p->size = (TABLE_SIZE-1)/2;
        new_node->size = TABLE_SIZE - p->size - 1;
        memcpy(new_node->items, &p->items[(TABLE_SIZE-1)/2+1], (new_node->size+1)*sizeof(btree_item));
        flushNode(bt, ph, new_node);
        return ph;
    }
}

void insertKey(BTree *bt, handle_t ph, void *key, handle_t value) {
    btree_node *p = getNode(bt, ph);
    int i;
    int ok = findKey(bt, p, key, &i);
    if (ok) i++;
    handle_t left_child = p->items[i].child, right_child = 0;
    if (!p->isLeaf) {
        insertKey(bt, left_child, key, value);
        btree_node *child = getNode(bt, left_child); 
        if (child->size < TABLE_SIZE) {
            putNode(bt, p);
            putNode(bt, child);
            return;
        }
        right_child = splitNode(bt, child, key);
        flushNode(bt, left_child, child);
    } else {
        //printf("find %d %d\n", i, ok);
        if (ok) {
            p->items[i-1].child = value;
            flushNode(bt, ph, p);
            return;
        }
        left_child = value;
        right_child = p->items[i].child;
    }
    p->size++;
    memmove(&p->items[i + 1], &p->items[i], (p->size-i)*sizeof(btree_item));
    memcpy(p->items[i].key, key, KEY_LENGTH);
    p->items[i].child = left_child;
    p->items[i + 1].child = right_child;
    flushNode(bt, ph, p);
}

void SetKey(BTree *bt, const void *key, handle_t value) {
    if (bt == NULL) return;
    uint8_t Key[KEY_LENGTH];
    convertKey(bt, key, value, Key);
    //memcpy(Key, key, KEY_LENGTH);
    handle_t left_child, right_child;
    if (bt->iroot != 0) {
        insertKey(bt, bt->iroot, Key, value);
        btree_node *p = getNode(bt, bt->iroot);
        //pout(p);
        if(p->size < TABLE_SIZE) {
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
    if(bt->iroot == 0) {
        new_node->isLeaf = 1;
        new_node->items[TABLE_SIZE+1].child = 0;
    }
    else new_node->isLeaf = 0;
    new_node->size = 1;
    memcpy(new_node->items[0].key, Key, KEY_LENGTH);
    new_node->items[0].child = left_child;
    new_node->items[1].child = right_child;
    //pout(new_node);
    flushNode(bt, ph, new_node);
    flushRoot(bt, ph);
}


void shiftRight(btree_node *p) {
    memmove(&p->items[1], &p->items[0], (p->size+1)*sizeof(btree_item));
}

void shiftLeft(btree_node *p) {
    memmove(&p->items[0], &p->items[1], p->size*sizeof(btree_item));
}

int isMerge(BTree *bt, btree_node *p, btree_node *child, int i, int isPred) {
    //printf("isMerge %d %d\n", i, isPred);
    btree_node *sibling;
    if (isPred) sibling = getNode(bt, p->items[i+1].child);
    else sibling = getNode(bt, p->items[i].child);
    if ( (child->isLeaf && sibling->size+child->size < TABLE_SIZE) ||
            (!child->isLeaf && sibling->size+child->size < TABLE_SIZE-1) ) {
        if (!isPred) {
            btree_node *tmp = child;
            child = sibling;
            sibling = tmp;
        }
        if (child->isLeaf) {
            memcpy(&(child->items[child->size]), &(sibling->items[0]), (sibling->size+1)*sizeof(btree_item));
            child->items[TABLE_SIZE+1].child = sibling->items[TABLE_SIZE+1].child;
            child->size += sibling->size;
        }
        else {
            memcpy(child->items[child->size].key, p->items[i].key, KEY_LENGTH);
            memcpy(&child->items[child->size+1], &sibling->items[0], (sibling->size+1)*sizeof(btree_item));
            child->size += sibling->size + 1;
        }
        //pout(child);
        flushNode(bt, p->items[i].child, child);
        putNode(bt, sibling);
        freeNode(bt, p->items[i+1].child);
        return 1;
    }
    if (!isPred) {
        shiftRight(child);
        if (!child->isLeaf) {
            memcpy(child->items[0].key, p->items[i].key, KEY_LENGTH);
            child->items[0].child = sibling->items[sibling->size].child;
            memcpy(p->items[i].key, sibling->items[sibling->size-1].key, KEY_LENGTH);
        }
        else {
            memcpy(child->items[0].key, sibling->items[sibling->size-1].key, KEY_LENGTH);
            child->items[0].child = sibling->items[sibling->size-1].child;
            sibling->items[sibling->size-1].child = 0;
            memcpy(p->items[i].key, child->items[0].key, KEY_LENGTH);
        }
        sibling->size--;
        child->size++;
        flushNode(bt, p->items[i].child, sibling);
        flushNode(bt, p->items[i+1].child, child);
    }
    else {
        if (!child->isLeaf) {
            memcpy(child->items[child->size].key, p->items[i].key, KEY_LENGTH);
            child->items[child->size+1].child = sibling->items[0].child;
            memcpy(p->items[i].key, sibling->items[0].key, KEY_LENGTH);
        }
        else {
            memcpy(child->items[child->size].key, sibling->items[0].key, KEY_LENGTH);
            child->items[child->size].child = sibling->items[0].child;
            memcpy(p->items[i].key, sibling->items[1].key, KEY_LENGTH);
        }
        shiftLeft(sibling);
        sibling->size--;
        child->size++;
        flushNode(bt, p->items[i].child, child);
        flushNode(bt, p->items[i+1].child, sibling);
    }
    return 0;
}

void moveLeft(btree_node *p, int i) {
    handle_t tmp = p->items[i].child;
    memmove(&p->items[i], &p->items[i + 1], (p->size-i)*sizeof(btree_item));
    p->size--;
    p->items[i].child = tmp;
}

void eraseKey(BTree *bt, handle_t ph, const void *key) {
    btree_node *p = getNode(bt, ph);
    //printf("erase %d ", ph);
    //pout(p);
    int i;
    int ok = findKey(bt, p, key, &i);
    if (ok) i++;
    handle_t left_child = p->items[i].child;
    if (!p->isLeaf) {
        eraseKey(bt, left_child, key);
        btree_node *child = getNode(bt, left_child); 
        //pout(child);
        if ( (child->isLeaf && child->size >= TABLE_SIZE/2) ||
                (!child->isLeaf && child->size >= (TABLE_SIZE-1)/2) ) {
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
        p->items[i].child = p->items[i+1].child;
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
    uint8_t Key[KEY_LENGTH];
    convertKey(bt, key, value, Key);
    eraseKey(bt, bt->iroot, Key);
    btree_node *p = getNode(bt, bt->iroot);
    //pout(p);
    //printf("%d\n", p->size);
    if (p->size == 0) {
        handle_t child = p->items[0].child;
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
            if (ok) ph = p->items[index].child;
            else ph = 0;
            putNode(bt, p);
            break;
        }
        if (ok) index++;
        ph = p->items[index].child;
        putNode(bt, p);
    }
    return ph;
}

int findKey(BTree *bt, btree_node *p, const void *key, int *index) {
    int l = -1;
    int r = p->size;
    while (l + 1 < r) {
        int mid = (l + r) >> 1;
        int ret = keyCmp(bt, p->items[mid].key, key);
        if (ret < 0) l = mid;
        else r = mid;
    }
    *index = r;
    return keyCmp(bt, p->items[*index].key, key) == 0;
}

void EnumLower_bound(BTreeEnum *bte, BTree *bt, const void *key) {
    EnumEnd(bte, bt);
    if(bt == NULL) return;
    if(bt->iroot == 0) return; 
    uint8_t Key[KEY_LENGTH];
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
            memcpy(bte->key, p->items[index].key, KEY_LENGTH);
            bte->value = p->items[index].child;
            putNode(bt, p);
            if (flag) MoveNext(bte);
            break;
        }
        if (isHit) index++;
        ph = p->items[index].child;
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
            memcpy(bte->key, p->items[0].key, KEY_LENGTH);
            //printf("%d\n", sizeof(bte->key));
            //printf("%d\n", sizeof(p->items[0].key));
            bte->value = p->items[0].child;
            putNode(bt, p);
            break;
        }
        ph = p->items[0].child;
        putNode(bt, p);
    }
}

void EnumEnd(BTreeEnum *bte, BTree *bt) {
    bte->store = 0;
    bte->id = 0;
    bte->index = 0;
    bte->isUnique = bt->isUnique;
}

void MoveNext(BTreeEnum *bte) {
    if(bte->id == 0) return;
    size_t len = 0;
    btree_node *p = read_blk(bte->store, bte->id, NULL, &len);
    assert(p != NULL);
    if (bte->index < p->size - 1) {
        bte->index++;
        memcpy(bte->key, p->items[bte->index].key, sizeof(bte->key));
        bte->value = p->items[bte->index].child;
        buf_put(bte->store, p);
    }
    else {
        bte->id = p->items[TABLE_SIZE+1].child;
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
        memcpy(bte->key, p->items[bte->index].key, sizeof(bte->key));
        bte->value = p->items[bte->index].child;
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
        memcpy(Key, key, KEY_LENGTH-sizeof(handle_t));
    }
    else {
        memcpy(Key, &value, sizeof(handle_t));
        memcpy(Key+sizeof(handle_t), key, KEY_LENGTH-sizeof(handle_t));
    }
}



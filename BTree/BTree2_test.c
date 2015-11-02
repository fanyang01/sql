#include "BTree.h"
#include "file.h"
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#define out(x, y) printf("%"PRIu64"\n", GetKey((x), (y)))
void output(BTree *bt) {
    BTreeEnum btn, invalid;
    EnumBegin(&btn, bt);
    EnumEnd(&invalid, bt);
    for(; !IsEqual(&btn, &invalid); MoveNext(&btn)) {
        printf("%s %"PRIu64"\n", BTKey(&btn), BTValue(&btn));
       //printf("%d %"PRIu64"\n", *(int*)BTKey(&btn), BTValue(&btn));
    }
}

int main(void)
{
    FILE *f = tmpfile();
    assert(f != NULL);
    int fd = fileno(f);
    ALLOC allocator;
    ALLOC *a = &allocator;
    assert(init_allocator(a, fd, 0) != 0);
    assert(init_allocator(a, fd, O_CREAT) == 0);
    assert(fsize(fd) == FLT_LEN);

    BTree bt;
    handle_t root = CreateBTree(&bt, a, 1, cmpStr);
    SetKey(&bt, "apple", 1);
    SetKey(&bt, "boy", 2);
    SetKey(&bt, "cat", 3);
    BTree *bt2 = malloc(sizeof(BTree));
    OpenBTree(bt2, a, 1, cmpStr, root);
    output(bt2);
    DeleteKey(bt2, "boy", 0);
    output(bt2);
    BTreeEnum btn, invalid;
    EnumLower_bound(&btn, bt2, "appl");
    EnumEnd(&invalid, bt2);
    for(; !IsEqual(&btn, &invalid); MoveNext(&btn)) {
        printf("%s %"PRIu64"\n", BTKey(&btn), BTValue(&btn));
    }
    return 0;
}

#include "BTree.h"
#include "file.h"
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#define out(x, y) printf("%"PRIu64"\n", GetKey(x, &y));
void output(BTree *bt) {
    BTreeEnum btn;
    BTreeEnum invalid;
    EnumBegin(&btn, bt);
    EnumEnd(&invalid, bt);
    for(; !IsEqual(&btn, &invalid); MoveNext(&btn)) {
       //printf("%s %"PRIu64"\n", BTKey(&btn), BTValue(&btn));
       printf("%d:%"PRIu64"\n", *(int*)BTKey(&btn), BTValue(&btn));
    }
    puts("Enum done");
}

#define oo(i) 
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
    handle_t root = CreateBTree(&bt, a, 1, 4, cmpInt);
    int i;
    for(i = 1; i <= 100000; i++) {
        SetKey(&bt, &i, i);
        //output(&bt);
        //getchar();
    }
    return 0;
    for(i = 1; i <= 10000; i++) {
        DeleteKey(&bt, &i, 100000-i+1);
    }
    return 0;
}

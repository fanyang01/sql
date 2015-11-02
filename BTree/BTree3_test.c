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
       printf("%d:%"PRIu64" ", *(int*)BTKey(&btn), BTValue(&btn));
    }
    puts("");
    puts("Enum done");
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
    handle_t root = CreateBTree(&bt, a, 0, cmpInt);
    int i;
    for(int i= 1; i <= 10; i++) {
        //a 5 1 a 5 4 a 6 5 a 6 7 a 8 7 a 8 8 a 9 8 a 9 1000 a 1 1 a 10 10
        char ch;int x, y;
        //x = i;
        //y = i;
        //SetKey(&bt, &x, y);
        scanf(" %c %d %d", &ch, &x, &y);
        if(ch == 'a') SetKey(&bt, &x, y);
        else if(ch == 'd') DeleteKey(&bt, &x, y);
        output(&bt);
    }
    BTree *bt2 = malloc(sizeof(BTree));
    OpenBTree(bt2, a, 0, cmpInt, root);
    output(bt2);
    BTreeEnum btn, invalid;
    int t1 = 5, t2 = 9;
    scanf("%d %d", &t1, &t2);
    EnumLower_bound(&btn, bt2, &t1);
    EnumUpper_bound(&invalid, bt2, &t2);
    for(; !IsEqual(&btn, &invalid); MoveNext(&btn)) {
        printf("%d %"PRIu64"\n", *(int*)BTKey(&btn), BTValue(&btn));
    }
    return 0;
}

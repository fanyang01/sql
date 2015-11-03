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
    //int key = 10;
    //SetKey(&bt, &key, 10);
    //out(&bt, key);
    //key = 6;
    //SetKey(&bt, &key, 6);
    //out(&bt, key);
    /*
    for(int i= 1; i <= 100; i++) {
        char ch;int x;
        scanf(" %c %d", &ch, &x);
        if(ch == 'a') SetKey(&bt, &x, x);
        else DeleteKey(&bt, &x);
    }
    */
    /*
    for (i = 1; i <= 10; i++) {
        int x;
        //scanf("%d", &x);
        x = i;
        SetKey(&bt, &x, i);
        output(&bt);
    }
   for (i = 1; i <= 10; i++) {
        int x;
        //scanf("%d", &x);
        x = 5;
        DeleteKey(&bt, &x);
        output(&bt);
    }
    int p = 4;
    //puts("-----------delete 4");
    //DeleteKey(&bt, &p);
    //puts("-----------delete 4 done");
    //DeleteKey(&bt, &p);
    p = 5;
    puts("------------add 5------------");
    SetKey(&bt, &p, 5);
    //ClearBTree(&bt);
    //output(&bt);
    puts("-----------------add 5 done----------------123123");
    for (i = 1; i <= 20; i++) {
        SetKey(&bt, &i, i);
        output(&bt);
    }
    */
    for(int i = 1; i <= 20; i++) SetKey(&bt, &i, i);
    //for(int i = 1; i<= 10; i++) out(&bt, i);
    //return 0;
    puts("123");
    output(&bt);
    //1 2 3...?
    int b[10];
    int n;
    scanf("%d", &n);
    for(int i = 0; i < n; i++) scanf("%d", &b[i]);
    //int b[] = {3, 7, 5}
    for(i = 0; i < n; i++) DeleteKey(&bt, &b[i], 0);
    output(&bt);
    // 1 2 0 4 0...?
    ClearBTree(&bt);
    output(&bt);
    //0 0 0 0...?
    for (i = 1; i <= 10; i++) SetKey(&bt, &i, i);
    output(&bt);
    //1 2 3...?
    BTree *bt2 = malloc(sizeof(BTree));
    OpenBTree(bt2, a, 1, 4, cmpInt, root);
    output(bt2);
    //1 2 3...?
    BTreeEnum btn, invalid;
    EnumBegin(&btn, bt2);
    EnumEnd(&invalid, bt2);
    for(; !IsEqual(&btn, &invalid); MoveNext(&btn)) {
        printf("%d %"PRIu64"\n", *(int*)BTKey(&btn), BTValue(&btn));
    }
    puts("");
    //1 2 3...?
    for(i = 0; i < n; i++) DeleteKey(bt2, &b[i], 0);
    int t1 = 5, t2 = 9;
    EnumLower_bound(&btn, bt2, &t1);
    EnumUpper_bound(&invalid, bt2, &t2);
    for(; !IsEqual(&btn, &invalid); MoveNext(&btn)) {
        printf("%d %"PRIu64"\n", *(int*)BTKey(&btn), BTValue(&btn));
    }
    //4 6...?
    return 0;
}

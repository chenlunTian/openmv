int aa[4] = { [2] = 3, [1] = 6 };
static short grid[3] [4] = { [0][0]=8, [0][1]=6,
                             [0][2]=4, [0][3]=1,
                             [2][0]=9, [2][1]=3,
                             [2][2]=1, [2][3]=1 };
int a[10] = {2, 4, [8]=9, 10};
int a[MAX] = {
    1, 3, 5, 7, 9, [MAX-5] = 8, 6, 4, 2, 0
};
struct {
    int table [3];
    struct {
        int a;
        int b;
    } parts;
} a[MAX] = {
    [2] = { .table = { 8, [1] = 7, 6 }, .parts = { .a = 0, 1 } }
};

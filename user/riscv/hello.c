#include <stdio.h>
#include <stdlib.h>

int main()
{
    int *a = (int *)malloc(1 << 20);
    memset(a, 0, 1 << 20);
    scanf("%d%d", a, a + 1);
    printf("%d\n", a[0] + a[1]);
    free(a);
    return 0;
}

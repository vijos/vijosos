#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__ ((constructor)) void foo(void)
{
	printf("foo is running and printf is available at this point\n");
}

int main()
{
    int *a = (int *)malloc(1 << 20);
    memset(a, 0, 1 << 20);
    scanf("%d%d", a, a + 1);
    printf("%d\n", a[0] + a[1]);
    free(a);
    return 0;
}

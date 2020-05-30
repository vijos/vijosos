#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

__attribute__ ((constructor)) void foo(void)
{
	printf("foo is running and printf is available at this point\n");
}

void zero1(void *buff, size_t len)
{
    uint8_t *end = (uint8_t *)buff + len;
    uint8_t *u8 = (uint8_t *)buff;
    for (; u8 != end; u8 += 1)
    {
        u8[0] = 0;
    }
}

void zero2(void *buff, size_t len)
{
    uint64_t *end = (uint64_t *)((uint8_t *)buff + len);
    uint64_t *u64 = (uint64_t *)buff;
    for (; u64 != end; u64 += 8)
    {
        u64[0] = 0;
        u64[1] = 0;
        u64[2] = 0;
        u64[3] = 0;
        u64[4] = 0;
        u64[5] = 0;
        u64[6] = 0;
        u64[7] = 0;
    }
}

#define SIZE (1 << 20)

int main()
{
    int *a = (int *)malloc(SIZE);

    int c;
    scanf("%d", &c);
    if (c == 0)
    {
        memset(a, 0, SIZE);
    }
    else if (c == 1)
    {
        zero1(a, SIZE);
    }
    else
    {
        zero2(a, SIZE);
    }

    scanf("%d%d", a, a + 1);
    printf("%d\n", a[0] + a[1]);
    free(a);
    return 0;
}

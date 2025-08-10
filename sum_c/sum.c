#include <stdio.h>

int main(void)
{
    int a[100];

    for (int i = 0; i < 100; i++)
    {
        a[i] = i + 1;
    }

    int sum[2] = {0, 0};
    for (int i = 0; i < 100; i++)
    {
        sum[a[i] & 1] += a[i];
    }

    printf("%d\n", sum);
}
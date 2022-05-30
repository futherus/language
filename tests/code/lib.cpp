#include <stdio.h>

extern "C" long getnum()
{
    long num = 0;
    scanf("%ld", &num);

    return num;
}

extern "C" long putnum(long num)
{
    printf("%ld\n", num);
    fflush(stdout);
    
    return 0;
}

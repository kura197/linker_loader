#include <stdio.h>

int extvalue = 20;
int extbssvalue;

int extfunc(int a, int b) {
    printf("extfunc()\n");
    int ret = a + b;
    return ret;
}
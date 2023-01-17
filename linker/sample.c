#include <stdio.h>

extern int extvalue;
extern int extbssvalue;
int extfunc(int a, int b);

static int value = 1234;
static int bssvalue;

//static int func() {
//    //printf("func()\n");
//    //printf("extvalue = %d\n", extvalue);
//    //printf("extbssvalue = %d\n", extbssvalue);
//    //printf("value = %d\n", value);
//    //printf("bssvalue = %d\n", bssvalue);
//    //int ret = extfunc(2, 3);
//    //printf("extfunc(2, 3) = %d\n", ret);
//    //return (extvalue + extbssvalue + value + bssvalue);
//    return value;
//}

int sample_main() {
    //printf("sample_main()\n");
    //int ret = func();
    //printf("func() = %d\n", ret);
    return value;
}

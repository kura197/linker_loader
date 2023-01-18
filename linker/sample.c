extern int extvalue;
extern int extbssvalue;
int extfunc(int a, int b);

static int value = 1234;
static int bssvalue;

static int func() {
    int ret = extfunc(200, 30);
    bssvalue = ret;
    // 230 + 13579 = 13809
    return ret + extvalue;
}

int sample_main() {
    int ret = func();
    // 13809 + 230 + 230 + 1234 = 15503
    return ret + extbssvalue + bssvalue + value;
}

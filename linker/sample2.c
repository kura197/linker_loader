int extvalue = 13579;
int extbssvalue;

int extfunc(int a, int b) {
    int ret = a + b;
    extbssvalue = ret;
    return ret;
}

#include <stdio.h>
#include <stdlib.h>

class test_class {
public:
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
    int g;
    int h;
    int i;
    int j;
    int k;
    int l;
    int m;
    int n;
    int o;
    int p;
    int q;
    int r;
    test_class() = default;
    ~test_class() = default;
    int test() {
        return a;
    }
};

int main(){
    printf("%d\n", __LINE__);
    int a = 0;
    printf("%d\n", __LINE__);
    int* pa = (int *)malloc(sizeof(int));
    printf("%d\n", __LINE__);
    int* pa2 = new int();
    printf("%d\n", __LINE__);
    test_class t;
    printf("%d\n", __LINE__);
    test_class* pt = new test_class();
    printf("%d\n", __LINE__);
    int* pn = nullptr;
    printf("%d\n", *pn);
    return 0;
}
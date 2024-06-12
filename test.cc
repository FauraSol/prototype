#include <cstdlib> // strtoul
#include <iostream>
#include <chrono>
#include <numa.h>
#include <vector>

#ifdef BOOST
#include <boost/stacktrace.hpp>
void print_stacktrace() {
    auto stack = boost::stacktrace::stacktrace();
    for (const auto& frame:stack ){
        int a;
    }
}

#elif defined(GLIBC)
#include <execinfo.h> /* backtrace, backtrace_symbols */
#include <stdio.h> /* printf */
void print_stacktrace(void) {
    char **strings;
    size_t i, size;
    enum Constexpr { MAX_SIZE = 1024 };
    void *array[MAX_SIZE];
    size = backtrace(array, MAX_SIZE);
    strings = backtrace_symbols(array, size);
    for (i = 0; i < size; i++)
        int a;
    free(strings);
}

#elif defined(GCC)
void print_stacktrace() {
    void* return_address = __builtin_return_address(0);
    // Dl_info info;
}
#else
void print_stacktrace() {}
#endif


void my_func_2(void) {
    print_stacktrace(); // line 6
}

void my_func_1(double f) {
    (void)f;
    my_func_2();
}

void my_func_1(int i) {
    (void)i;
    my_func_2(); // line 16
}

int main(int argc, char **argv) {
    long long unsigned int n;
    if (argc > 1) {
        n = std::strtoul(argv[1], NULL, 0);
    } else {
        n = 1;
    }
    void **v = (void **)malloc(n * sizeof(void*));
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; i++)
    {
        auto ptr = numa_alloc_onnode(4096, 0);
        v[i] = ptr;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    for(int i=0;i<n;i++){
        numa_free(v[i], 4096);
    }
    std::cerr << duration.count() / n << std::endl;
    return 0;
}
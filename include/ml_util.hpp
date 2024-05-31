#ifndef ML_UTIL_H
#define ML_UTIL_H

#include <dlfcn.h>
#include <cxxabi.h>
#include <immintrin.h>
#include "evaluation_queue.hpp"

enum FEATURES{
    F_PAGE_ID = 0,
    F_PAGE_OFFSET,
    F_SIZE,
    F_OP,
    F_PC,
    NUM_OF_FEATURES
};

typedef struct Features{
    int test = 1;
}Features;

inline const char *get_funcname(const char *src);

#endif
#include "ml_util.hpp"
inline const char *get_funcname(const char *src) {
    int status = 99;
    const char *f = abi::__cxa_demangle(src, nullptr, nullptr, &status);
    return f == nullptr ? src : f;
}
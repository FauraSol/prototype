#include <numa.h>
#include <vector>
#include <iostream>

#include "cmdline.hpp"
#include "concurrent_hashmap.hpp"
#include <boost/stacktrace.hpp> //-lboost_stacktrace_backtrace -ldl
#include <mlpack/core.hpp>
#include <mlpack.hpp>

using std::vector, std::cout, std::endl;

int main(){
    mlpack::HoeffdingTree tree;
    arma::Row<int> vec({1,2,3,4,5});
    tree.Classify(vec);
    auto res = boost::stacktrace::stacktrace();
    cout << res << endl;
}

//g++ test.cc -std=c++17 -O0 -g  -lpthread -lnuma -o numa_access_test -I../include 
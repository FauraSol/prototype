#include <time.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#include "mempool.hpp"  // 注释这行比较系统malloc与memory pool的性能
#include "log.hpp"
#include "cmdline.hpp"
struct Node{
    char data[64];
};

struct TestParam{
    int thread_num;
    int interations;

} param;

void test_fn(){
    const int DATA_N = param.interations / param.thread_num;
    Node mem[DATA_N];
    
    std::shuffle(mem, mem + DATA_N, std::default_random_engine(time(0)));
}



int main(int argc, char* argv[]){
    cmdline::parser parser;
    parser.add<int>("thread", 't', "thread number");
    parser.add<int>("interations", 'i', "interations");

    parser.parse_check(argc, argv);
    param.thread_num = parser.get<int>("thread");
    param.interations = parser.get<int>("interations");


    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    return 0;
}
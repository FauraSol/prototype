#include <numa.h>
#include <vector>
#include <iostream>

#include "cmdline.hpp"
#include "concurrent_hashmap.hpp"

using std::vector, std::cout, std::endl;

int main(){
    vector<void*> addr_node0;
    vector<void*> addr_node1;
    if (numa_available() < 0) {
        printf("NUMA not supported\n");
    }
    for(int i=0;i<10;i++){
        void* addr = numa_alloc_onnode(sizeof(uint64_t), 0);
        addr_node0.push_back(addr);
    }

    for(int i=0;i<10;i++){
        void* addr = numa_alloc_onnode(sizeof(uint64_t), 1);
        addr_node1.push_back(addr);
    }

    for(int i = 0; i < 10; i++){
        cout << addr_node0[i] << " ";
    }
    cout << endl;
    for(int i = 0; i < 10; i++){
        cout << addr_node1[i] << " ";
    }
    cout << endl;
    for(int i = 0; i < 10; i++){
        numa_free(addr_node0[i], sizeof(uint64_t));
        numa_free(addr_node1[i], sizeof(uint64_t));
    }
    return 0;
}

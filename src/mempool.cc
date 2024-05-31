#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include "mempool.hpp"
using std::thread, std::vector, std::string;
void test_fn(MemoryPool<size_t>& pool){
    for(int i=0;i<10000;i++){
        auto ptr = pool.newElement();
        *ptr = std::hash<std::thread::id>{}(std::this_thread::get_id());
    }
}

class KVPair_for_mempool{
    char key[26];
    char value[102];
public:
    KVPair_for_mempool(string key, string value){
        strcpy(this->key, key.c_str());
        strcpy(this->value, value.c_str());
    }
    KVPair_for_mempool(){
    }
    friend std::ostream& operator<<(std::ostream& os, const KVPair_for_mempool& kv){
        os << "kv size:"<< std::dec <<sizeof(KVPair_for_mempool) << std::endl;
        os << kv.key << ":" << kv.value << std::endl;
        os << "self address:" << std::hex  << &kv << std::endl;
        os << "key address:" << std::hex  << &kv.key << std::endl;
        os << "value address:" << std::hex  << &kv.value << std::endl;
        return os;
    }
    ~KVPair_for_mempool() = default;
};

int main()
{
  
  return 0;
}
#include "mempool.hpp"


using std::string;
using std::vector;

class KVPair_for_RDMA_mempool{
    char key_[26];
    char value_[102];
public:
    KVPair_for_RDMA_mempool(string key, string value){
        static_assert(sizeof(KVPair_for_RDMA_mempool) == 26 + 102, "sizeof(KVPair_for_RDMA_mempool) != 128");
        strcpy(this->key_, key.c_str());
        strcpy(this->value_, value.c_str());
    }
    KVPair_for_RDMA_mempool(const char* key, const char* value){
        strcpy(this->key_, key);
        strcpy(this->value_, value);
    }
    const char* key() const { return key_; }
    const char* value() const { return value_; }
    void set_value(const char* value) { 
        strcpy(this->value_, value); 
    }
    KVPair_for_RDMA_mempool(){
    }
    ~KVPair_for_RDMA_mempool() = default;
};

namespace ycsbc {
class my_rdma_db  {
public:

    my_rdma_db() : pool_() {
    }
    ~my_rdma_db() = default;

    void Init();

protected:
    const char *CopyString(const std::string &str) {
        char *value = new char[str.length() + 1];
        strcpy(value, str.c_str());
        return value;
    }
private:
    MemoryPool<KVPair_for_RDMA_mempool> pool_;
};
void my_rdma_db::Init() {
        return ;
    }
};


int main(int argc, char **argv) {
    long long unsigned int n;
    if (argc > 1) {
        n = std::strtoul(argv[1], NULL, 0);
    } else {
        n = 1;
    }
    MemoryPool<size_t> pool;
    size_t* val;
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; i++)
    {
        val = pool.newElement(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cerr << duration.count() / n << std::endl;
    return 0;
}

//g++ mempool.cc -O0 -std=c++17 -ggdb -I../include -lnuma -finstrument-functions -ldl -rdynamic -lpci -lpthread -o mempool_test
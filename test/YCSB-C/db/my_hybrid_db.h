#ifndef YCSB_C_MY_HYBRID_DB_H
#define YCSB_C_MY_HYBRID_DB_H
#include "core/db.h"
#include <string>
#include <vector>
#include "mempool.hpp"
#include "lib/string_hashtable.h"
#include "lib/lock_stl_hashtable.h"

using std::string;
using std::vector;

class KVPair_for_mempool{
    char key_[26];
    char value_[102];
public:
    KVPair_for_mempool(string key, string value){
        static_assert(sizeof(KVPair_for_mempool) == 26 + 102, "sizeof(KVPair_for_mempool) != 128");
        strcpy(this->key_, key.c_str());
        strcpy(this->value_, value.c_str());
    }
    KVPair_for_mempool(const char* key, const char* value){
        assert(strlen(key) < 26);
        assert(strlen(value) < 102);
        strcpy(this->key_, key);
        strcpy(this->value_, value);
    }
    const char* key() const { return key_; }
    const char* value() const { return value_; }
    void set_value(const char* value) { 
        assert(strlen(value) < 102);
        strcpy(this->value_, value); 
    }
    KVPair_for_mempool(){
    }
    ~KVPair_for_mempool() = default;
};

namespace ycsbc {
class my_hybrid_db : public DB {
public:
    typedef vmp::StringHashtable<KVPair_for_mempool *> AddrHashtable;

    my_hybrid_db() : pool_(), addrHashtable_(nullptr) {
        addrHashtable_ = nullptr;
        addrHashtable_ = new vmp::LockStlHashtable<KVPair_for_mempool *>;
        assert(addrHashtable_!=nullptr);
        fprintf(stderr, "addrHashtable_: %p\n", addrHashtable_);
    }
    ~my_hybrid_db() = default;

    void Init();

    int Read(const string &table, const string &key, const vector<string> *fields, vector<KVPair> &result);

    int Scan(const string &table, const string &key, int len, const vector<string> *fields, vector<vector<KVPair>> &result);
    
    int Update(const string &table, const string &key, vector<KVPair> &values);

    int Insert(const string &table, const string &key, vector<KVPair> &values);

    int Delete(const string &table, const string &key);
protected:
    const char *CopyString(const std::string &str) {
        char *value = new char[str.length() + 1];
        strcpy(value, str.c_str());
        return value;
    }
private:
    MemoryPool<KVPair_for_mempool> pool_;
    AddrHashtable *addrHashtable_;
};


};// namespace ycsbc

#endif
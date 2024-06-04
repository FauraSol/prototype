#include "db/my_hybrid_db.h"

#include <string>
#include <vector>
#include "lib/string_hashtable.h"

using std::string;
using std::vector;
using vmp::StringHashtable;

namespace ycsbc {

    void my_hybrid_db::Init() {
        return ;
    }

    int my_hybrid_db::Read(const string &table, const string &key, const vector<string> *fields, vector<KVPair> &result) {
        string key_index(table + key);
        result.clear();
        auto addr = static_cast<KVPair_for_mempool*>(addrHashtable_->Get(key_index.c_str()));
        result.push_back(std::make_pair(addr->key(), addr->value()));
        printf("Read key: %s, value: %s\n", key_index.c_str(), addr->value());
        return DB::kOK;
    }

    int my_hybrid_db::Scan(const string &table, const string &key, int len, const vector<string> *fields, vector<vector<KVPair>> &result) {
        string key_index(table + key);
        vector<AddrHashtable::KVPair> key_pairs = addrHashtable_->Entries(key_index.c_str(), len);
        result.clear();
        for (auto &key_pair : key_pairs) {
            auto addr = static_cast<KVPair_for_mempool*>(key_pair.second);
            vector<KVPair> field_values;
            field_values.push_back(std::make_pair(addr->key(), addr->value()));
            printf("Scan key: %s, value: %s\n", key_index.c_str(), addr->value());
            result.push_back(field_values);
        }
        return DB::kOK;
    }

    int my_hybrid_db::Update(const string &table, const string &key, vector<KVPair> &values) {
        string key_index(table + key);
        auto addr = static_cast<KVPair_for_mempool*>(addrHashtable_->Get(key_index.c_str()));
        printf("Update key: %s, old value: %s\n", key_index.c_str(), addr->value());
        const char* value = CopyString(values[0].second);
        addr->set_value(value);
        printf("Update key: %s, new value: %s\n", key_index.c_str(), addr->value());
        return DB::kOK;
    }

    int my_hybrid_db::Insert(const string &table, const string &key, vector<KVPair> &values) {
        string key_index(table + key);
        assert(values.size()==1);
        for(KVPair& kv: values) {
            const char* key = CopyString(kv.first);
            const char* value = CopyString(kv.second);
            const char* addr_key = CopyString(key_index);
            printf("Insert key: %s, value: %s\n", key_index.c_str(), value);
            auto addr = pool_.newElement(key,value);
            bool ok = addrHashtable_->Insert(addr_key, addr);
        }
        return DB::kOK;
    }

    int my_hybrid_db::Delete(const string &table, const string &key) {
        string key_index(table + key);
        auto addr = static_cast<KVPair_for_mempool*>(addrHashtable_->Get(key_index.c_str()));
        pool_.deleteElement(addr);
        addrHashtable_->Remove(key_index.c_str());
        return DB::kOK;
    }
}
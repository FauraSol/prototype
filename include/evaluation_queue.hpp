#ifndef EVALUATION_QUEUE_H
#define EVALUATION_QUEUE_H

#include <list>
#include <unordered_map>
// #include <priority_queue>
#include <string>
#include <type_traits>
#include <vector>
#include <optional>
#include <cmath>
#include <cstdint>
#include "ml_util.hpp"

namespace my_ml{

typedef struct EQ_item{
    Feat_vec_t feat_vec;
    uint64_t last_access_;
    double heatness_;
    uint64_t is_hot_;
    EQ_item(Feat_vec_t &&feat_vec, uint64_t ts) : heatness_(0), last_access_(ts), is_hot_(false){
        this->feat_vec = std::move(feat_vec);
    }
    EQ_item() = default;
    EQ_item(const EQ_item &) = default;
    EQ_item(EQ_item &&) = default;
    EQ_item &operator=(const EQ_item &) = default;
    EQ_item &operator=(EQ_item &&) = default;
} EQ_item_t, feature_t;

// using eq_item = std::unordered_map<std::string, std::string>;

/**
 * 1. 先进先出
 * 2. 随机修改
 * 3. 出队时判断热度
*/
template <typename Key>
class EvaluationQueue {
private:
    using KeyList = std::list<Key>;
    using KeyMapIter = typename KeyList::iterator;
    using MapType = std::unordered_map<Key, std::pair<EQ_item, KeyMapIter>>;

    auto update_value(Key key, EQ_item &&val) -> void {
        auto &[value, iter] = map[key];
        auto factor = std::exp(alpha_ * (val.last_access_ - value.last_access_));
        value.heatness_ = value.heatness_ * factor + heat_;
        value.last_access_ = val.last_access_;
    }

    auto put_or_update(const Key &key, EQ_item &&value) -> void {
        if (map.count(key) > 0) {
            // 如果 key 已存在,则更新值
            update_value(key, std::move(value));
        } else {
            // 如果 key 不存在,则添加新的键值对
            keys.emplace_back(key);
            value.heatness_ = heat_;
            map.emplace(key, std::make_pair(value, --keys.end()));
        }
    }

    auto get(const Key &key) const -> EQ_item {
        return map.at(key).first;
    }

    auto erase(const Key &key) -> void {
        if (map.count(key) > 0) {
            auto [val, iter] = map[key];
            keys.erase(iter);
            map.erase(key);
        }
    }

    auto pop() -> std::optional<EQ_item> {
        if (keys.empty()) {
            return std::nullopt;
        }
        auto key = keys.front();
        auto [val, iter] = map[key];
        keys.pop_front();
        auto factor = std::exp(alpha_ * (ts_ - val.last_access_));
        val.heatness_ *= factor;
        map.erase(key);
        return std::move(val);
    }

    auto contains(const Key &key) const -> bool {
        return map.count(key) > 0;
    }

    auto begin() -> KeyMapIter {
        return keys.begin();
    }

    auto end() -> KeyMapIter {
        return keys.end();
    }

    size_t size() const {
        return keys.size();
    }

private:
    KeyList keys;
    MapType map;

public:
    EvaluationQueue(size_t max_size=100, double hot_thred=0.0125, bool is_training=true, double alpha=-0.03, double heat=200.0) :
        max_size_(max_size), ts_(0), hot_thred_(hot_thred), is_training_(is_training), alpha_(alpha), heat_(heat) {
    }
    ~EvaluationQueue() = default;
    size_t max_size_;
    uint64_t ts_;
    double hot_thred_;
    bool is_training_;
    double alpha_;
    double heat_;

    auto enqueue(const Key &key, Feat_vec_t &&value) -> std::optional<EQ_item>;
    auto dequeue() -> std::optional<EQ_item>;
};

} // namespace my_ml
namespace my_ml {
template <typename Key>
auto EvaluationQueue<Key>::enqueue(const Key &key, Feat_vec_t &&value) -> std::optional<EQ_item> {
    EQ_item item(std::move(value), ++ts_);
    put_or_update(key, std::move(item));
    std::optional<EQ_item> ret;
    if (this->size() > max_size_) {
        ret = dequeue();
    }
    return ret;
}
template <typename Key>
auto EvaluationQueue<Key>::dequeue() -> std::optional<EQ_item> {
    auto val = pop();
    if (!val.has_value()) {
        printf("Queue is empty\n");
    }
    val.value().is_hot_ = val.value().heatness_ >= hot_thred_;
    return val;
}
};

#endif
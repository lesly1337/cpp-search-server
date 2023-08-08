#pragma once
#include <mutex>
#include <map>
#include <vector>
using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct SubMap {
        std::mutex m;
        std::map<Key, Value> submap_;
    };
    std::vector<SubMap> data;
    int GetIndex(Key key) {
        int index = key;
        return abs(index) % data.size();
    }
    
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
        
        Access(const Key& key, SubMap& submap)
            : guard(submap.m)
            , ref_to_value(submap.submap_[key]) {
        }
    };

    explicit ConcurrentMap(size_t bucket_count) : data(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        auto& val = data[GetIndex(key)];
        return {key, val};
    };

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, submap] : data) {
            std::lock_guard guard(mutex);
            result.insert(submap.begin(), submap.end());
        }
        return result;
    };
    //Добавлен метод Erase
    void Erase(const Key& key) {
        std::lock_guard guard(data[GetIndex(key)].m);
        data[GetIndex(key)].submap_.erase(key);
    }
};
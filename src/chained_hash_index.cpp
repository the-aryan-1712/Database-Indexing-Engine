#include "../include/chained_hash_index.h"

ChainedHashIndex::ChainedHashIndex(size_t initial_buckets, double max_load)
    : buckets_(initial_buckets), max_load_(max_load) {}

size_t ChainedHashIndex::bucket_for(int key) const {
    return std::hash<int>{}(key) % buckets_.size();
}

void ChainedHashIndex::rehash(size_t new_bucket_count) {
    std::vector<std::list<Entry>> old;
    old.swap(buckets_);
    buckets_.resize(new_bucket_count);
    count_ = 0;
    for (auto& chain : old)
        for (auto& e : chain)
            insert(e.key, e.value);
}

void ChainedHashIndex::insert(int key, const std::string& value) {
    size_t b = bucket_for(key);
    for (auto& e : buckets_[b]) {
        if (e.key == key) { e.value = value; return; }   // update
    }
    buckets_[b].push_back({key, value});
    ++count_;
    if (static_cast<double>(count_) / buckets_.size() > max_load_)
        rehash(buckets_.size() * 2);
}

std::optional<std::string> ChainedHashIndex::search(int key) const {
    size_t b = bucket_for(key);
    for (auto& e : buckets_[b])
        if (e.key == key) return e.value;
    return std::nullopt;
}

bool ChainedHashIndex::remove(int key) {
    size_t b = bucket_for(key);
    for (auto it = buckets_[b].begin(); it != buckets_[b].end(); ++it) {
        if (it->key == key) {
            buckets_[b].erase(it);
            --count_;
            return true;
        }
    }
    return false;
}

size_t ChainedHashIndex::size() const { return count_; }

size_t ChainedHashIndex::memoryUsage() const {
    size_t mem = sizeof(*this);
    mem += buckets_.capacity() * sizeof(std::list<Entry>);
    for (auto& chain : buckets_) {
        // Each list node: Entry + two pointers
        for (auto& e : chain) {
            mem += sizeof(Entry) + 2 * sizeof(void*);
            mem += e.value.capacity();
        }
    }
    return mem;
}

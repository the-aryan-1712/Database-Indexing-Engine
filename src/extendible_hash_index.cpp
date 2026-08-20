#include "../include/extendible_hash_index.h"
#include <functional>
#include <cassert>

ExtendibleHashIndex::ExtendibleHashIndex(int bucket_capacity)
    : global_depth_(1), bucket_capacity_(bucket_capacity) {
    // Start with 2 buckets (directory size = 2^1 = 2)
    directory_.resize(2);
    for (auto& ptr : directory_) {
        ptr = std::make_shared<Bucket>();
        ptr->local_depth = 1;
    }
}

size_t ExtendibleHashIndex::hash_val(int key) const {
    return std::hash<int>{}(key);
}

size_t ExtendibleHashIndex::dir_index(int key) const {
    return hash_val(key) & ((1ULL << global_depth_) - 1);
}

void ExtendibleHashIndex::insert(int key, const std::string& value) {
    size_t idx = dir_index(key);
    auto& bucket = directory_[idx];

    // Check for existing key - update
    for (auto& [k, v] : bucket->entries) {
        if (k == key) { v = value; return; }
    }

    // If bucket has room, just insert
    if (static_cast<int>(bucket->entries.size()) < bucket_capacity_) {
        bucket->entries.emplace_back(key, value);
        ++count_;
        return;
    }

    // Bucket is full - split
    split_bucket(idx);
    // Retry insert after split
    insert(key, value);
}

void ExtendibleHashIndex::split_bucket(size_t idx) {
    auto old_bucket = directory_[idx];
    int old_depth = old_bucket->local_depth;

    if (old_depth == global_depth_) {
        // Double the directory
        size_t old_size = directory_.size();
        directory_.resize(old_size * 2);
        for (size_t i = 0; i < old_size; ++i)
            directory_[i + old_size] = directory_[i];
        ++global_depth_;
    }

    // Create two new buckets
    auto b0 = std::make_shared<Bucket>();
    auto b1 = std::make_shared<Bucket>();
    b0->local_depth = old_depth + 1;
    b1->local_depth = old_depth + 1;

    size_t bit = 1ULL << old_depth;  // the new discriminating bit

    // Redistribute entries
    for (auto& [k, v] : old_bucket->entries) {
        if (hash_val(k) & bit)
            b1->entries.emplace_back(k, v);
        else
            b0->entries.emplace_back(k, v);
    }

    // Update directory pointers
    for (size_t i = 0; i < directory_.size(); ++i) {
        if (directory_[i] == old_bucket) {
            if (i & bit)
                directory_[i] = b1;
            else
                directory_[i] = b0;
        }
    }
}

std::optional<std::string> ExtendibleHashIndex::search(int key) const {
    size_t idx = dir_index(key);
    for (auto& [k, v] : directory_[idx]->entries)
        if (k == key) return v;
    return std::nullopt;
}

bool ExtendibleHashIndex::remove(int key) {
    size_t idx = dir_index(key);
    auto& entries = directory_[idx]->entries;
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it->first == key) {
            entries.erase(it);
            --count_;
            return true;
        }
    }
    return false;
}

size_t ExtendibleHashIndex::size() const { return count_; }

size_t ExtendibleHashIndex::memoryUsage() const {
    size_t mem = sizeof(*this);
    mem += directory_.capacity() * sizeof(std::shared_ptr<Bucket>);

    // Count unique buckets
    std::vector<Bucket*> seen;
    for (auto& ptr : directory_) {
        bool found = false;
        for (auto* s : seen) { if (s == ptr.get()) { found = true; break; } }
        if (!found) {
            seen.push_back(ptr.get());
            mem += sizeof(Bucket);
            mem += ptr->entries.capacity() * sizeof(std::pair<int, std::string>);
            for (auto& [k, v] : ptr->entries)
                mem += v.capacity();
        }
    }
    return mem;
}

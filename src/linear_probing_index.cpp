#include "../include/linear_probing_index.h"
#include <functional>

LinearProbingIndex::LinearProbingIndex(size_t initial_capacity)
    : table_(initial_capacity), capacity_(initial_capacity) {}

size_t LinearProbingIndex::probe(int key) const {
    return std::hash<int>{}(key) % capacity_;
}

void LinearProbingIndex::resize(size_t new_cap) {
    std::vector<Slot> old;
    old.swap(table_);
    table_.resize(new_cap);
    capacity_ = new_cap;
    count_ = 0;
    for (auto& s : old)
        if (s.state == OCCUPIED)
            insert(s.key, s.value);
}

void LinearProbingIndex::insert(int key, const std::string& value) {
    if (count_ * 10 >= capacity_ * 7)   // load > 0.7
        resize(capacity_ * 2);

    size_t idx = probe(key);
    size_t first_deleted = capacity_;     // sentinel

    for (size_t i = 0; i < capacity_; ++i) {
        size_t pos = (idx + i) % capacity_;
        if (table_[pos].state == EMPTY) {
            size_t target = (first_deleted < capacity_) ? first_deleted : pos;
            table_[target] = {key, value, OCCUPIED};
            ++count_;
            return;
        }
        if (table_[pos].state == DELETED && first_deleted == capacity_)
            first_deleted = pos;
        if (table_[pos].state == OCCUPIED && table_[pos].key == key) {
            table_[pos].value = value;   // update
            return;
        }
    }
    // table full (shouldn't happen due to resize)
    size_t target = (first_deleted < capacity_) ? first_deleted : idx;
    table_[target] = {key, value, OCCUPIED};
    ++count_;
}

std::optional<std::string> LinearProbingIndex::search(int key) const {
    size_t idx = probe(key);
    for (size_t i = 0; i < capacity_; ++i) {
        size_t pos = (idx + i) % capacity_;
        if (table_[pos].state == EMPTY) return std::nullopt;
        if (table_[pos].state == OCCUPIED && table_[pos].key == key)
            return table_[pos].value;
    }
    return std::nullopt;
}

bool LinearProbingIndex::remove(int key) {
    size_t idx = probe(key);
    for (size_t i = 0; i < capacity_; ++i) {
        size_t pos = (idx + i) % capacity_;
        if (table_[pos].state == EMPTY) return false;
        if (table_[pos].state == OCCUPIED && table_[pos].key == key) {
            table_[pos].state = DELETED;
            --count_;
            return true;
        }
    }
    return false;
}

size_t LinearProbingIndex::size() const { return count_; }

size_t LinearProbingIndex::memoryUsage() const {
    size_t mem = sizeof(*this);
    mem += table_.capacity() * sizeof(Slot);
    for (auto& s : table_)
        if (s.state == OCCUPIED) mem += s.value.capacity();
    return mem;
}

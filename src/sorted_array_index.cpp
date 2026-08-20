#include "../include/sorted_array_index.h"
#include <algorithm>

// ── lower-bound helper (returns index of first entry with key >= target) ──
size_t SortedArrayIndex::lowerBound(int key) const {
    size_t lo = 0, hi = data_.size();
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (data_[mid].key < key) lo = mid + 1;
        else                      hi = mid;
    }
    return lo;
}

void SortedArrayIndex::insert(int key, const std::string& value) {
    size_t pos = lowerBound(key);
    if (pos < data_.size() && data_[pos].key == key) {
        data_[pos].value = value;          // update existing
        return;
    }
    data_.insert(data_.begin() + static_cast<long>(pos), {key, value});
}

std::optional<std::string> SortedArrayIndex::search(int key) const {
    size_t pos = lowerBound(key);
    if (pos < data_.size() && data_[pos].key == key)
        return data_[pos].value;
    return std::nullopt;
}

bool SortedArrayIndex::remove(int key) {
    size_t pos = lowerBound(key);
    if (pos < data_.size() && data_[pos].key == key) {
        data_.erase(data_.begin() + static_cast<long>(pos));
        return true;
    }
    return false;
}

size_t SortedArrayIndex::size() const { return data_.size(); }

size_t SortedArrayIndex::memoryUsage() const {
    size_t mem = sizeof(*this);
    mem += data_.capacity() * sizeof(Entry);
    for (auto& e : data_) mem += e.value.capacity();
    return mem;
}

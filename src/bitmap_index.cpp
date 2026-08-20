#include "../include/bitmap_index.h"

// Bitmaps are lazily sized — only extended to `records_.size()` on demand
void BitmapIndex::ensureBitmapSize(int key) {
    bitmaps_[key].resize(records_.size(), false);
}

void BitmapIndex::insert(int key, const std::string& value) {
    // Check if key already exists - update (scan the key's bitmap only)
    auto it = bitmaps_.find(key);
    if (it != bitmaps_.end()) {
        for (size_t i = 0; i < it->second.size(); ++i) {
            if (it->second[i] && active_[i]) {
                records_[i].value = value;   // update
                return;
            }
        }
    }

    // Add new record at next position
    size_t pos = records_.size();
    records_.push_back({key, value});
    active_.push_back(true);

    // Only extend + set the bitmap for THIS key (lazy — don't touch others)
    auto& bm = bitmaps_[key];
    bm.resize(pos + 1, false);
    bm[pos] = true;
    ++count_;
}

std::optional<std::string> BitmapIndex::search(int key) const {
    auto it = bitmaps_.find(key);
    if (it == bitmaps_.end()) return std::nullopt;
    for (size_t i = 0; i < it->second.size(); ++i) {
        if (it->second[i] && i < active_.size() && active_[i])
            return records_[i].value;
    }
    return std::nullopt;
}

bool BitmapIndex::remove(int key) {
    auto it = bitmaps_.find(key);
    if (it == bitmaps_.end()) return false;
    for (size_t i = 0; i < it->second.size(); ++i) {
        if (it->second[i] && i < active_.size() && active_[i]) {
            it->second[i] = false;
            active_[i] = false;
            --count_;
            return true;
        }
    }
    return false;
}

std::vector<size_t> BitmapIndex::queryAnd(int key1, int key2) const {
    std::vector<size_t> result;
    auto it1 = bitmaps_.find(key1);
    auto it2 = bitmaps_.find(key2);
    if (it1 == bitmaps_.end() || it2 == bitmaps_.end()) return result;

    size_t len = std::min({it1->second.size(), it2->second.size(), active_.size()});
    for (size_t i = 0; i < len; ++i)
        if (it1->second[i] && it2->second[i] && active_[i])
            result.push_back(i);
    return result;
}

size_t BitmapIndex::size() const { return count_; }

size_t BitmapIndex::memoryUsage() const {
    size_t mem = sizeof(*this);
    mem += records_.capacity() * sizeof(Slot);
    for (auto& r : records_) mem += r.value.capacity();
    mem += active_.capacity() / 8;
    for (auto& [k, bm] : bitmaps_)
        mem += sizeof(int) + bm.capacity() / 8;
    return mem;
}

#pragma once
#include "index_interface.h"
#include <vector>
#include <list>
#include <string>
#include <optional>
#include <functional>

// ── Hash index with separate chaining ──────────────────────────────────
// Average O(1) search / insert / delete; rehashes when load factor > 0.75
class ChainedHashIndex : public IIndex {
public:
    explicit ChainedHashIndex(size_t initial_buckets = 16,
                              double max_load = 0.75);

    void        insert(int key, const std::string& value) override;
    std::optional<std::string> search(int key) const override;
    bool        remove(int key) override;
    size_t      size() const override;
    size_t      memoryUsage() const override;
    std::string name() const override { return "ChainedHashIndex"; }

private:
    struct Entry { int key; std::string value; };
    std::vector<std::list<Entry>> buckets_;
    size_t count_ = 0;
    double max_load_;

    size_t bucket_for(int key) const;
    void   rehash(size_t new_bucket_count);
};

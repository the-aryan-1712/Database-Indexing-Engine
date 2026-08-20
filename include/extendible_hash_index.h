#pragma once
#include "index_interface.h"
#include <vector>
#include <memory>
#include <string>
#include <optional>

// ── Extendible hashing ─────────────────────────────────────────────────
// Directory doubles when a bucket overflows and its local_depth == global_depth.
// Bucket splits use the next bit of the hash.
class ExtendibleHashIndex : public IIndex {
public:
    explicit ExtendibleHashIndex(int bucket_capacity = 4);

    void        insert(int key, const std::string& value) override;
    std::optional<std::string> search(int key) const override;
    bool        remove(int key) override;
    size_t      size() const override;
    size_t      memoryUsage() const override;
    std::string name() const override { return "ExtendibleHashIndex"; }

private:
    struct Bucket {
        int local_depth;
        std::vector<std::pair<int, std::string>> entries;
    };

    int global_depth_;
    int bucket_capacity_;
    size_t count_ = 0;
    std::vector<std::shared_ptr<Bucket>> directory_;

    size_t hash_val(int key) const;
    size_t dir_index(int key) const;
    void   split_bucket(size_t idx);
};

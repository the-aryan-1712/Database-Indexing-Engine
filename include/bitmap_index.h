#pragma once
#include "index_interface.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>

// ── Bitmap index ───────────────────────────────────────────────────────
// Maintains a bitvector per distinct key value.
// Efficient for low-cardinality attributes; supports bitwise AND/OR queries.
class BitmapIndex : public IIndex {
public:
    void        insert(int key, const std::string& value) override;
    std::optional<std::string> search(int key) const override;
    bool        remove(int key) override;
    size_t      size() const override;
    size_t      memoryUsage() const override;
    std::string name() const override { return "BitmapIndex"; }

    // ── bitmap-specific helpers ──
    // AND two key bitmaps, return matching record positions
    std::vector<size_t> queryAnd(int key1, int key2) const;

private:
    // Records stored by position; a deleted slot has active_[i] == false.
    struct Slot { int key; std::string value; };
    std::vector<Slot>  records_;
    std::vector<bool>  active_;

    // key-value → bitvector  (length == records_.size())
    std::unordered_map<int, std::vector<bool>> bitmaps_;
    size_t count_ = 0;

    void ensureBitmapSize(int key);
};

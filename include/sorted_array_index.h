#pragma once
#include "index_interface.h"
#include <vector>
#include <string>
#include <optional>

// ── Ordered index backed by a sorted std::vector ───────────────────────
// Search : O(log n)  via binary search
// Insert : O(n)      due to shifting
// Delete : O(n)      due to shifting
class SortedArrayIndex : public IIndex {
public:
    void        insert(int key, const std::string& value) override;
    std::optional<std::string> search(int key) const override;
    bool        remove(int key) override;
    size_t      size() const override;
    size_t      memoryUsage() const override;
    std::string name() const override { return "SortedArrayIndex"; }

private:
    struct Entry { int key; std::string value; };
    std::vector<Entry> data_;

    // Returns iterator to entry with key >= target.
    size_t lowerBound(int key) const;
};

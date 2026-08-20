#pragma once
#include "index_interface.h"
#include <vector>
#include <string>
#include <optional>

// ── Open-addressing hash index with linear probing ─────────────────────
// Uses tombstone (DELETED) markers to keep probe chains intact.
// Resizes when load factor > 0.7.
class LinearProbingIndex : public IIndex {
public:
    explicit LinearProbingIndex(size_t initial_capacity = 16);

    void        insert(int key, const std::string& value) override;
    std::optional<std::string> search(int key) const override;
    bool        remove(int key) override;
    size_t      size() const override;
    size_t      memoryUsage() const override;
    std::string name() const override { return "LinearProbingIndex"; }

private:
    enum State : uint8_t { EMPTY, OCCUPIED, DELETED };
    struct Slot {
        int         key   = 0;
        std::string value;
        State       state = EMPTY;
    };

    std::vector<Slot> table_;
    size_t count_    = 0;
    size_t capacity_;

    size_t probe(int key) const;
    void   resize(size_t new_cap);
};

#pragma once
#include <string>
#include <optional>
#include <cstdint>
#include <algorithm>
#include <initializer_list>

// ── Abstract base class for all 1-D index structures ───────────────────
class IIndex {
public:
    virtual ~IIndex() = default;

    // Insert a (key, value) pair.  If key already exists, update value.
    virtual void insert(int key, const std::string& value) = 0;

    // Search for key.  Returns the associated value or std::nullopt.
    virtual std::optional<std::string> search(int key) const = 0;

    // Remove the entry with the given key.  Returns true if found & removed.
    virtual bool remove(int key) = 0;

    // Number of stored entries.
    virtual size_t size() const = 0;

    // Approximate heap memory used by the data structure (bytes).
    virtual size_t memoryUsage() const = 0;

    // Human-readable name of the index type.
    virtual std::string name() const = 0;
};

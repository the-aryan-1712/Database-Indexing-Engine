#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <memory>
#include <iomanip>

#include "sorted_array_index.h"
#include "chained_hash_index.h"
#include "linear_probing_index.h"
#include "extendible_hash_index.h"
#include "bitmap_index.h"
#include "btree_index.h"
#include "bplus_tree_index.h"
#include "rtree_index.h"

// ── Helpers ────────────────────────────────────────────────────────────
static int g_pass = 0, g_fail = 0;

#define CHECK(cond, msg)                                                 \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::cerr << "  FAIL: " << msg << "  (" << __FILE__          \
                      << ":" << __LINE__ << ")\n";                       \
            ++g_fail;                                                    \
        } else { ++g_pass; }                                             \
    } while (0)

// ── Generic test suite that works for every IIndex ─────────────────────
static void testGeneric(IIndex& idx) {
    std::cout << "  [generic] Testing " << idx.name() << " ...\n";

    // Empty
    CHECK(idx.size() == 0, "empty size");
    CHECK(!idx.search(42).has_value(), "search empty");
    CHECK(!idx.remove(42), "remove empty");

    // Single insert + search
    idx.insert(10, "ten");
    CHECK(idx.size() == 1, "size after 1 insert");
    CHECK(idx.search(10).value() == "ten", "search single");

    // Update
    idx.insert(10, "TEN");
    CHECK(idx.size() == 1, "size after update");
    CHECK(idx.search(10).value() == "TEN", "search after update");

    // Multiple inserts
    idx.insert(20, "twenty");
    idx.insert(5, "five");
    idx.insert(15, "fifteen");
    CHECK(idx.size() == 4, "size after 4 inserts");
    CHECK(idx.search(5).value() == "five", "search 5");
    CHECK(idx.search(15).value() == "fifteen", "search 15");
    CHECK(idx.search(20).value() == "twenty", "search 20");

    // Delete
    CHECK(idx.remove(15), "remove 15");
    CHECK(idx.size() == 3, "size after delete");
    CHECK(!idx.search(15).has_value(), "search deleted 15");
    CHECK(!idx.remove(15), "double remove");

    // Delete remaining
    CHECK(idx.remove(10), "remove 10");
    CHECK(idx.remove(5), "remove 5");
    CHECK(idx.remove(20), "remove 20");
    CHECK(idx.size() == 0, "size after delete all");
}

// ── Bulk test ──────────────────────────────────────────────────────────
static void testBulk(IIndex& idx, int n = 2000) {
    std::cout << "  [bulk]    Testing " << idx.name() << " with " << n << " keys ...\n";

    std::vector<int> keys(n);
    for (int i = 0; i < n; ++i) keys[i] = i;
    std::mt19937 rng(42);
    std::shuffle(keys.begin(), keys.end(), rng);

    // Insert all
    for (int k : keys)
        idx.insert(k, "v" + std::to_string(k));

    CHECK((int)idx.size() == n, "bulk size");

    // Verify all present
    bool all_found = true;
    for (int k : keys) {
        auto v = idx.search(k);
        if (!v.has_value() || v.value() != "v" + std::to_string(k)) {
            all_found = false; break;
        }
    }
    CHECK(all_found, "bulk search all");

    // Delete half
    std::shuffle(keys.begin(), keys.end(), rng);
    int half = n / 2;
    for (int i = 0; i < half; ++i)
        CHECK(idx.remove(keys[i]), "bulk remove");

    CHECK((int)idx.size() == n - half, "bulk size after half delete");

    // Verify remaining
    for (int i = half; i < n; ++i) {
        auto v = idx.search(keys[i]);
        CHECK(v.has_value(), "bulk search remaining");
    }

    // Verify deleted are gone
    for (int i = 0; i < half; ++i)
        CHECK(!idx.search(keys[i]).has_value(), "bulk deleted gone");

    // Clean up remaining
    for (int i = half; i < n; ++i) idx.remove(keys[i]);
    CHECK(idx.size() == 0, "bulk empty after full delete");
}

// ── B+ Tree range query test ───────────────────────────────────────────
static void testBPlusRange() {
    std::cout << "  [range]   Testing B+Tree range query ...\n";
    BPlusTreeIndex bpt(32);
    for (int i = 0; i < 100; ++i)
        bpt.insert(i, "v" + std::to_string(i));

    auto r = bpt.rangeSearch(25, 35);
    CHECK((int)r.size() == 11, "range [25,35] size");
    for (auto& [k, v] : r)
        CHECK(k >= 25 && k <= 35, "range key in bounds");
}

// ── R-Tree spatial query test ──────────────────────────────────────────
static void testRTreeSpatial() {
    std::cout << "  [spatial] Testing R-Tree range query ...\n";
    RTreeIndex rt(10, 4);
    for (int i = 0; i < 50; ++i)
        rt.insertPoint({(double)i, (double)(i * 2)}, "p" + std::to_string(i));

    MBR query{10.0, 20.0, 30.0, 60.0};
    auto results = rt.rangeQuery(query);
    CHECK(!results.empty(), "spatial query not empty");
    for (auto& [pt, v] : results)
        CHECK(query.contains(pt), "spatial result in query rect");
}

// ── Bitmap AND query test ──────────────────────────────────────────────
static void testBitmapAnd() {
    std::cout << "  [bitmap]  Testing Bitmap AND query ...\n";
    BitmapIndex bm;
    bm.insert(1, "a");
    bm.insert(2, "b");
    bm.insert(3, "c");
    // AND of different keys should be empty (unique keys)
    auto res = bm.queryAnd(1, 2);
    CHECK(res.empty(), "AND of disjoint keys empty");
}

// ── Main ───────────────────────────────────────────────────────────────
int main() {
    std::cout << "======================================\n";
    std::cout << " IndexEngine — Correctness Tests\n";
    std::cout << "======================================\n\n";

    // Construct all indices
    std::vector<std::unique_ptr<IIndex>> indices;
    indices.push_back(std::make_unique<SortedArrayIndex>());
    indices.push_back(std::make_unique<ChainedHashIndex>());
    indices.push_back(std::make_unique<LinearProbingIndex>());
    indices.push_back(std::make_unique<ExtendibleHashIndex>(4));
    indices.push_back(std::make_unique<BitmapIndex>());
    indices.push_back(std::make_unique<BTreeIndex>(3));        // small t for thorough split/merge
    indices.push_back(std::make_unique<BPlusTreeIndex>(6));    // small order for thorough testing
    indices.push_back(std::make_unique<RTreeIndex>(10, 4));

    for (auto& idx : indices) {
        std::cout << "\n── " << idx->name() << " ──\n";
        testGeneric(*idx);
    }

    // Bulk tests (fresh instances)
    std::cout << "\n── Bulk tests ──\n";
    {
        SortedArrayIndex i1; testBulk(i1, 2000);
        ChainedHashIndex i2; testBulk(i2, 5000);
        LinearProbingIndex i3; testBulk(i3, 5000);
        ExtendibleHashIndex i4(4); testBulk(i4, 5000);
        BitmapIndex i5; testBulk(i5, 2000);
        BTreeIndex i6(3); testBulk(i6, 5000);
        BPlusTreeIndex i7(6); testBulk(i7, 5000);
        RTreeIndex i8(10, 4); testBulk(i8, 1000);
    }

    // Specialised tests
    std::cout << "\n── Specialised tests ──\n";
    testBPlusRange();
    testRTreeSpatial();
    testBitmapAnd();

    // ── Space overhead check ───────────────────────────────────────────
    // In a real DB: data file stores full records (~200 bytes each),
    // index file stores only keys + record pointers + structure.
    std::cout << "\n── Space overhead check (index < data file?) ──\n";
    std::cout << "  (Simulated record size = 200 bytes)\n";
    {
        auto checkSpace = [](IIndex& idx, int n) {
            constexpr size_t RECORD_SIZE = 200;
            size_t data_bytes = (size_t)n * RECORD_SIZE;

            for (int i = 0; i < n; ++i)
                idx.insert(i, "v" + std::to_string(i));

            // Index-only overhead: total mem minus value-string objects
            // plus record pointers (what a real index would store)
            size_t total_mem = idx.memoryUsage();
            size_t index_overhead = total_mem
                                  - (size_t)n * sizeof(std::string)
                                  + (size_t)n * sizeof(void*);

            bool smaller = (index_overhead < data_bytes);
            std::cout << "  " << std::left << std::setw(22) << idx.name()
                      << "  idx=" << std::setw(10) << index_overhead
                      << "  data=" << std::setw(10) << data_bytes
                      << "  " << (smaller ? "YES (index < data)" : "NO  (index >= data)")
                      << "\n";
            for (int i = 0; i < n; ++i) idx.remove(i);
        };

        const int N = 10000;
        SortedArrayIndex      s1; checkSpace(s1, N);
        ChainedHashIndex      s2; checkSpace(s2, N);
        LinearProbingIndex    s3; checkSpace(s3, N);
        ExtendibleHashIndex   s4(64); checkSpace(s4, N);
        BitmapIndex           s5; checkSpace(s5, N);
        BTreeIndex            s6(64); checkSpace(s6, N);
        BPlusTreeIndex        s7(128); checkSpace(s7, N);
        RTreeIndex            s8(50, 20); checkSpace(s8, N);
    }

    // Summary
    std::cout << "\n======================================\n";
    std::cout << " Results: " << g_pass << " passed, " << g_fail << " failed\n";
    std::cout << "======================================\n";
    return g_fail > 0 ? 1 : 0;
}

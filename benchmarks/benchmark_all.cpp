#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <algorithm>
#include <functional>
#include <filesystem>

#include "sorted_array_index.h"
#include "chained_hash_index.h"
#include "linear_probing_index.h"
#include "extendible_hash_index.h"
#include "bitmap_index.h"
#include "btree_index.h"
#include "bplus_tree_index.h"
#include "rtree_index.h"

// ── Timer helper ───────────────────────────────────────────────────────
using Clock = std::chrono::high_resolution_clock;

inline double usElapsed(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::micro>(end - start).count();
}

// ── Factory ────────────────────────────────────────────────────────────
struct IndexFactory {
    std::string name;
    std::function<std::unique_ptr<IIndex>()> make;
};

static std::vector<IndexFactory> factories() {
    return {
        {"SortedArrayIndex",    []{ return std::make_unique<SortedArrayIndex>(); }},
        {"ChainedHashIndex",    []{ return std::make_unique<ChainedHashIndex>(); }},
        {"LinearProbingIndex",  []{ return std::make_unique<LinearProbingIndex>(); }},
        {"ExtendibleHashIndex", []{ return std::make_unique<ExtendibleHashIndex>(64); }},
        {"BitmapIndex",         []{ return std::make_unique<BitmapIndex>(); }},
        {"BTreeIndex",          []{ return std::make_unique<BTreeIndex>(64); }},
        {"B+TreeIndex",         []{ return std::make_unique<BPlusTreeIndex>(128); }},
        {"RTreeIndex",          []{ return std::make_unique<RTreeIndex>(50, 20); }},
    };
}

// ── Benchmark one index at a given data size ───────────────────────────
struct BenchResult {
    std::string index_name;
    int         data_size;
    double      insert_us;       // total μs for all inserts
    double      search_us;       // total μs for all searches
    double      delete_us;       // total μs for all deletes
    size_t      memory_bytes;    // index space after all inserts
    size_t      data_bytes;      // raw data size (what the index covers)
    bool        smaller_than_data; // true if index < raw data
};

static BenchResult benchmark(const IndexFactory& factory, int n) {
    auto idx = factory.make();

    // In a real DBMS each record (row) is much larger than our tiny test values.
    // We simulate a realistic record size of 200 bytes per row (typical for a
    // table with several columns).  The DATA FILE stores full records; the
    // INDEX FILE stores only keys + record-pointers + structural overhead.
    constexpr size_t RECORD_SIZE = 200;          // simulated bytes per record
    size_t data_bytes = (size_t)n * RECORD_SIZE; // total data-file size

    // Prepare shuffled keys
    std::vector<int> keys(n);
    for (int i = 0; i < n; ++i) keys[i] = i;
    std::mt19937 rng(12345);
    std::shuffle(keys.begin(), keys.end(), rng);

    // ── INSERT ──
    auto t0 = Clock::now();
    for (int k : keys)
        idx->insert(k, "val_" + std::to_string(k));
    auto t1 = Clock::now();
    double insert_us = usElapsed(t0, t1);

    // ── INDEX OVERHEAD ──
    // memoryUsage() counts EVERYTHING including stored values.  A real index
    // would store (key, record_pointer) instead of (key, value_string).
    // Subtract the value-string objects and add back a record pointer per entry.
    size_t total_mem = idx->memoryUsage();
    size_t index_overhead = total_mem
                          - (size_t)n * sizeof(std::string)   // remove value objects
                          + (size_t)n * sizeof(void*);        // add record pointers
    bool smaller = (index_overhead < data_bytes);

    // ── SEARCH (all keys, random order) ──
    std::shuffle(keys.begin(), keys.end(), rng);
    auto t2 = Clock::now();
    for (int k : keys)
        idx->search(k);
    auto t3 = Clock::now();
    double search_us = usElapsed(t2, t3);

    // ── DELETE (all keys, random order) ──
    std::shuffle(keys.begin(), keys.end(), rng);
    auto t4 = Clock::now();
    for (int k : keys)
        idx->remove(k);
    auto t5 = Clock::now();
    double delete_us = usElapsed(t4, t5);

    return {factory.name, n, insert_us, search_us, delete_us, index_overhead, data_bytes, smaller};
}

// ── Main ───────────────────────────────────────────────────────────────
int main() {
    // Standard sizes and reduced sizes for O(n²) indices
    std::vector<int> full_sizes   = {1000, 10000, 100000};
    std::vector<int> medium_sizes = {1000, 10000, 50000};

    auto facs = factories();
    std::vector<BenchResult> results;

    std::cout << "======================================================\n";
    std::cout << " IndexEngine — Benchmark Suite\n";
    std::cout << "======================================================\n\n";

    for (auto& fac : facs) {
        // Use reduced sizes for indices with O(n²) insert/delete
        bool is_slow = (fac.name == "SortedArrayIndex" ||
                        fac.name == "BitmapIndex" ||
                        fac.name == "RTreeIndex");
        auto& sizes = is_slow ? medium_sizes : full_sizes;

        for (int n : sizes) {
            std::cout << "  Benchmarking " << std::setw(22) << std::left
                      << fac.name << "  N=" << std::setw(8) << n << " ... " << std::flush;
            auto r = benchmark(fac, n);
            results.push_back(r);
            std::cout << "done\n";
        }
    }

    // ── Print table ────────────────────────────────────────────────────
    std::cout << "\n";
    std::cout << std::string(140, '-') << "\n";
    std::cout << std::left
              << std::setw(22) << "Index"
              << std::setw(9)  << "N"
              << std::right
              << std::setw(14) << "Insert(us)"
              << std::setw(14) << "Search(us)"
              << std::setw(14) << "Delete(us)"
              << std::setw(14) << "IdxMem(KB)"
              << std::setw(14) << "DataSize(KB)"
              << std::setw(14) << "Idx<Data?"
              << "\n";
    std::cout << std::string(140, '-') << "\n";

    for (auto& r : results) {
        std::cout << std::left
                  << std::setw(22) << r.index_name
                  << std::setw(9)  << r.data_size
                  << std::right << std::fixed << std::setprecision(1)
                  << std::setw(14) << r.insert_us
                  << std::setw(14) << r.search_us
                  << std::setw(14) << r.delete_us
                  << std::setw(14) << (r.memory_bytes / 1024.0)
                  << std::setw(14) << (r.data_bytes / 1024.0)
                  << std::setw(14) << (r.smaller_than_data ? "Yes" : "No")
                  << "\n";
    }
    std::cout << std::string(140, '-') << "\n";

    // ── Write CSV ──────────────────────────────────────────────────────
    std::filesystem::create_directories("results");
    std::ofstream csv("results/benchmark_report.csv");
    csv << "Index,DataSize,InsertTime_us,SearchTime_us,DeleteTime_us,SpaceOverheadBytes,DataSizeBytes,IsSmallerThanData\n";
    for (auto& r : results) {
        csv << r.index_name << ","
            << r.data_size << ","
            << std::fixed << std::setprecision(2)
            << r.insert_us << ","
            << r.search_us << ","
            << r.delete_us << ","
            << r.memory_bytes << ","
            << r.data_bytes << ","
            << (r.smaller_than_data ? "Yes" : "No") << "\n";
    }
    csv.close();
    std::cout << "\nCSV report written to: results/benchmark_report.csv\n";

    return 0;
}

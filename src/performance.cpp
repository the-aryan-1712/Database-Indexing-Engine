#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <algorithm>
#include <fstream>

#include "../include/sorted_array_index.h"
#include "../include/chained_hash_index.h"
#include "../include/linear_probing_index.h"
#include "../include/extendible_hash_index.h"
#include "../include/bitmap_index.h"
#include "../include/btree_index.h"
#include "../include/bplus_tree_index.h"
#include "../include/rtree_index.h"

using namespace std;
using namespace std::chrono;

const vector<int> TEST_SIZES = {
    100,
    250,
    500,
    750,
    1000,
    2000,
    3000,
    4000,
    5000,
    7500,
    10000,
    15000,
    20000,
    25000,
    40000,
    50000,
    75000,
    100000,
    150000,
    200000,
};

const int TRIALS = 5;

template<typename Func>
double measureTime(Func operation) {
    auto start = high_resolution_clock::now();
    operation();
    auto stop = high_resolution_clock::now();
    return duration_cast<microseconds>(stop - start).count();
}

int main() {
    ofstream file("sorted_array_results.csv"); // change filename per index

    file << "N,AvgInsert,WorstInsert,AvgSearch,WorstSearch,AvgDelete,WorstDelete,Memory\n";

    for (int N : TEST_SIZES) {
        cout << "Testing N = " << N << "\n";

        vector<double> insertTimes;
        vector<double> searchTimes;
        vector<double> deleteTimes;
        vector<size_t> memoryValues;

        for (int t = 0; t < TRIALS; ++t) {
        SortedArrayIndex idx; // change constructor per index

            vector<int> keys(N);
            for (int i = 0; i < N; ++i) keys[i] = i;

            mt19937 rng(t + 42);
            shuffle(keys.begin(), keys.end(), rng);

            double insertTime = measureTime([&]() {
                for (int k : keys)
                    idx.insert(k, "v" + to_string(k));
            });

            double searchTime = measureTime([&]() {
                for (int k : keys)
                    idx.search(k);
            });

            memoryValues.push_back(idx.memoryUsage());

            shuffle(keys.begin(), keys.end(), rng);

            double deleteTime = measureTime([&]() {
                for (int k : keys)
                    idx.remove(k);
            });

            insertTimes.push_back(insertTime);
            searchTimes.push_back(searchTime);
            deleteTimes.push_back(deleteTime);
        }

        auto avg = [](const vector<double>& v) {
            double s = 0;
            for (double x : v) s += x;
            return s / v.size();
        };

        auto worst = [](const vector<double>& v) {
            return *max_element(v.begin(), v.end());
        };

        auto avgMem = [](const vector<size_t>& v) {
            size_t s = 0;
            for (size_t x : v) s += x;
            return s / v.size();
        };

        file << N << ","
             << avg(insertTimes) << ","
             << worst(insertTimes) << ","
             << avg(searchTimes) << ","
             << worst(searchTimes) << ","
             << avg(deleteTimes) << ","
             << worst(deleteTimes) << ","
             << avgMem(memoryValues)
             << "\n";
    }

    file.close();
    cout << "Benchmark complete. CSV generated.\n";
}
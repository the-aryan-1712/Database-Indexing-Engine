# Index Engine Performance Benchmark

This project benchmarks multiple database indexing structures and compares their practical time and space behavior against theoretical complexity.

The goal is to study how different indexing methods behave as the total number of records `N` increases from very small datasets to very large datasets.

# Indexed Structures Covered

The benchmark supports the following index implementations:

* Sorted Array Index
* Chained Hash Index
* Linear Probing Hash Index
* Extendible Hash Index
* Bitmap Index
* B-Tree Index
* B+ Tree Index
* R-Tree Index

Each index is tested independently using a separate benchmark file.


# Compilation

Use performance.cpp present in tests folder as a generic file for all indexes and modify it for the index you want to test.

Compile each benchmark separately.

Example for B-Tree:

```bash
g++ benchmark_btree.cpp btree_index.cpp -O2 -std=c++17 -o bench_btree
```

Example for B+ Tree:

```bash
g++ benchmark_bplus_tree.cpp bplus_tree_index.cpp -O2 -std=c++17 -o bench_bplus
```

Use:

```bash
-O2
```

for optimized and more realistic benchmark results.

---

# Running

Run the executable using:

```bash
./bench_btree
```

The terminal will display progress like:

```text
Testing N = 100
Testing N = 500
Testing N = 1000
...
```

After completion, the CSV file will be generated in the same folder.
#pragma once
#include "index_interface.h"
#include "record.h"
#include <vector>
#include <string>
#include <optional>

// ── R-Tree (2-D spatial index) ─────────────────────────────────────────
// Uses MBRs (minimum bounding rectangles).
// Quadratic-cost split algorithm.
// Implements IIndex by mapping int key → Point2D(key, key*37 % 10007).
class RTreeIndex : public IIndex {
public:
    explicit RTreeIndex(int max_entries = 50, int min_entries = 20);
    ~RTreeIndex() override;

    // IIndex — key is mapped to a deterministic 2-D point internally
    void        insert(int key, const std::string& value) override;
    std::optional<std::string> search(int key) const override;
    bool        remove(int key) override;
    size_t      size() const override;
    size_t      memoryUsage() const override;
    std::string name() const override { return "RTreeIndex"; }

    // ── spatial helpers ──
    void insertPoint(const Point2D& pt, const std::string& value);
    std::vector<std::pair<Point2D, std::string>> rangeQuery(const MBR& query) const;
    bool removePoint(const Point2D& pt);

public:
    struct Node;  // forward declaration

    struct Entry {
        MBR         mbr;
        std::string value;       // only for leaf entries
        Node* child = nullptr;   // only for internal entries
    };

    struct Node {
        bool is_leaf = true;
        std::vector<Entry> entries;
    };

private:
    int    max_entries_;
    int    min_entries_;
    Node*  root_;
    size_t count_     = 0;
    size_t node_count_= 0;

    Point2D keyToPoint(int key) const;

    Node* makeNode(bool leaf);
    void  freeNode(Node* n);

    // choose subtree for insertion
    Node* chooseLeaf(Node* node, const MBR& mbr);

    // split
    void splitNode(Node* node, Node* parent);
    std::pair<int,int> pickSeeds(const std::vector<Entry>& entries);

    // adjust MBRs upward
    MBR computeMBR(Node* node) const;

    // insert helpers
    void insertEntry(Node* node, const Entry& entry, Node* parent);

    // search helpers
    void rangeSearch(Node* node, const MBR& query,
                     std::vector<std::pair<Point2D, std::string>>& results) const;

    // remove helpers
    bool removeEntry(Node* node, const Point2D& pt, Node* parent);
    void condenseTree(Node* node, Node* parent);

    // parent lookup (simple recursion since we don't store parent ptrs)
    Node* findParent(Node* current, Node* child) const;

    size_t nodeMemory(Node* n) const;
};

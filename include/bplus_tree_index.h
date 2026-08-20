#pragma once
#include "index_interface.h"
#include <vector>
#include <string>
#include <optional>

// ── B+-Tree ────────────────────────────────────────────────────────────
// Internal nodes: routing keys only.  All data lives in linked leaves.
// Supports efficient range queries via leaf linked list.
class BPlusTreeIndex : public IIndex {
public:
    explicit BPlusTreeIndex(int order = 128);   // max keys per node
    ~BPlusTreeIndex() override;

    void        insert(int key, const std::string& value) override;
    std::optional<std::string> search(int key) const override;
    bool        remove(int key) override;
    size_t      size() const override;
    size_t      memoryUsage() const override;
    std::string name() const override { return "B+TreeIndex"; }

    // Range query [lo, hi] inclusive
    std::vector<std::pair<int, std::string>> rangeSearch(int lo, int hi) const;

private:
    struct Node {
        bool is_leaf = false;
        std::vector<int> keys;

        // leaf fields
        std::vector<std::string> values;   // parallel to keys (leaf only)
        Node* next = nullptr;              // leaf linked list

        // internal fields
        std::vector<Node*> children;       // size = keys.size()+1 (internal only)
    };

    int    order_;         // max keys per node
    int    min_keys_;      // ceil(order/2) - 1
    Node*  root_;
    size_t count_     = 0;
    size_t node_count_= 0;

    Node*  makeNode(bool leaf);
    void   freeNode(Node* n);
    Node*  findLeaf(int key) const;

    void   insertIntoLeaf(Node* leaf, int key, const std::string& val);
    void   insertIntoParent(Node* left, int key, Node* right);
    void   splitLeaf(Node* leaf);
    void   splitInternal(Node* node);

    // deletion
    bool   removeFromLeaf(Node* leaf, int key);
    void   fixAfterDelete(Node* node, Node* parent);
    Node*  findParent(Node* current, Node* child) const;
    int    findChildIndex(Node* parent, Node* child) const;
    void   redistributeOrMergeLeaf(Node* node, Node* parent, int idx);
    void   redistributeOrMergeInternal(Node* node, Node* parent, int idx);
    size_t nodeMemory(Node* n) const;
};

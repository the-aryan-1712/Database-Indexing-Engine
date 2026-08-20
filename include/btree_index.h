#pragma once
#include "index_interface.h"
#include <vector>
#include <string>
#include <optional>

// ── B-Tree ─────────────────────────────────────────────────────────────
// Keys AND record pointers in BOTH internal and leaf nodes.
// Minimum degree t: every non-root node has [t-1, 2t-1] keys.
class BTreeIndex : public IIndex {
public:
    explicit BTreeIndex(int min_degree = 64);
    ~BTreeIndex() override;

    void        insert(int key, const std::string& value) override;
    std::optional<std::string> search(int key) const override;
    bool        remove(int key) override;
    size_t      size() const override;
    size_t      memoryUsage() const override;
    std::string name() const override { return "BTreeIndex"; }

private:
    struct Node {
        bool is_leaf = true;
        std::vector<int>         keys;
        std::vector<std::string> values;   // parallel to keys
        std::vector<Node*>       children; // size = keys.size()+1 for internal
    };

    int    t_;       // minimum degree
    Node*  root_;
    size_t count_ = 0;
    size_t node_count_ = 0;

    // helpers
    Node* makeNode(bool leaf);
    void  freeNode(Node* n);
    void  splitChild(Node* parent, int idx);
    void  insertNonFull(Node* n, int key, const std::string& val);
    std::optional<std::string> searchNode(Node* n, int key) const;

    // deletion helpers (CLRS style)
    bool  removeFromNode(Node* n, int key);
    void  removeFromLeaf(Node* n, int idx);
    void  removeFromInternal(Node* n, int idx);
    int   getPredecessorKey(Node* n, std::string& val);
    int   getSuccessorKey(Node* n, std::string& val);
    void  merge(Node* parent, int idx);
    void  fill(Node* parent, int idx);
    void  borrowFromPrev(Node* parent, int idx);
    void  borrowFromNext(Node* parent, int idx);
    size_t nodeMemory(Node* n) const;
};

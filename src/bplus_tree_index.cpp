#include "../include/bplus_tree_index.h"
#include <algorithm>
#include <cassert>

BPlusTreeIndex::BPlusTreeIndex(int order)
    : order_(order), min_keys_((order + 1) / 2 - 1), root_(nullptr) {}

BPlusTreeIndex::~BPlusTreeIndex() { freeNode(root_); }

BPlusTreeIndex::Node* BPlusTreeIndex::makeNode(bool leaf) {
    auto* n = new Node(); n->is_leaf = leaf; ++node_count_; return n;
}

void BPlusTreeIndex::freeNode(Node* n) {
    if (!n) return;
    if (!n->is_leaf) for (auto* c : n->children) freeNode(c);
    --node_count_; delete n;
}

BPlusTreeIndex::Node* BPlusTreeIndex::findLeaf(int key) const {
    if (!root_) return nullptr;
    Node* cur = root_;
    while (!cur->is_leaf) {
        int i = 0;
        while (i < (int)cur->keys.size() && key >= cur->keys[i]) ++i;
        cur = cur->children[i];
    }
    return cur;
}

std::optional<std::string> BPlusTreeIndex::search(int key) const {
    Node* leaf = findLeaf(key);
    if (!leaf) return std::nullopt;
    for (int i = 0; i < (int)leaf->keys.size(); ++i)
        if (leaf->keys[i] == key) return leaf->values[i];
    return std::nullopt;
}

std::vector<std::pair<int,std::string>> BPlusTreeIndex::rangeSearch(int lo, int hi) const {
    std::vector<std::pair<int,std::string>> res;
    Node* leaf = findLeaf(lo);
    while (leaf) {
        for (int i = 0; i < (int)leaf->keys.size(); ++i) {
            if (leaf->keys[i] > hi) return res;
            if (leaf->keys[i] >= lo) res.emplace_back(leaf->keys[i], leaf->values[i]);
        }
        leaf = leaf->next;
    }
    return res;
}

void BPlusTreeIndex::insertIntoLeaf(Node* leaf, int key, const std::string& val) {
    int i = 0;
    while (i < (int)leaf->keys.size() && leaf->keys[i] < key) ++i;
    leaf->keys.insert(leaf->keys.begin() + i, key);
    leaf->values.insert(leaf->values.begin() + i, val);
}

void BPlusTreeIndex::insert(int key, const std::string& value) {
    if (!root_) {
        root_ = makeNode(true);
        root_->keys.push_back(key);
        root_->values.push_back(value);
        ++count_; return;
    }
    Node* leaf = findLeaf(key);
    for (int i = 0; i < (int)leaf->keys.size(); ++i)
        if (leaf->keys[i] == key) { leaf->values[i] = value; return; }
    insertIntoLeaf(leaf, key, value);
    ++count_;
    if ((int)leaf->keys.size() > order_ - 1) splitLeaf(leaf);
}

void BPlusTreeIndex::splitLeaf(Node* leaf) {
    Node* nw = makeNode(true);
    int sp = (int)leaf->keys.size() / 2;
    nw->keys.assign(leaf->keys.begin()+sp, leaf->keys.end());
    nw->values.assign(leaf->values.begin()+sp, leaf->values.end());
    leaf->keys.resize(sp); leaf->values.resize(sp);
    nw->next = leaf->next; leaf->next = nw;
    insertIntoParent(leaf, nw->keys.front(), nw);
}

void BPlusTreeIndex::insertIntoParent(Node* left, int key, Node* right) {
    if (left == root_) {
        Node* nr = makeNode(false);
        nr->keys.push_back(key);
        nr->children.push_back(left);
        nr->children.push_back(right);
        root_ = nr; return;
    }
    Node* par = findParent(root_, left);
    int i = findChildIndex(par, left);
    par->keys.insert(par->keys.begin()+i, key);
    par->children.insert(par->children.begin()+i+1, right);
    if ((int)par->keys.size() > order_ - 1) splitInternal(par);
}

void BPlusTreeIndex::splitInternal(Node* node) {
    Node* nw = makeNode(false);
    int sp = (int)node->keys.size() / 2;
    int up = node->keys[sp];
    nw->keys.assign(node->keys.begin()+sp+1, node->keys.end());
    nw->children.assign(node->children.begin()+sp+1, node->children.end());
    node->keys.resize(sp); node->children.resize(sp+1);
    if (node == root_) {
        Node* nr = makeNode(false);
        nr->keys.push_back(up);
        nr->children.push_back(node);
        nr->children.push_back(nw);
        root_ = nr;
    } else {
        insertIntoParent(node, up, nw);
    }
}

bool BPlusTreeIndex::removeFromLeaf(Node* leaf, int key) {
    for (int i = 0; i < (int)leaf->keys.size(); ++i) {
        if (leaf->keys[i] == key) {
            leaf->keys.erase(leaf->keys.begin()+i);
            leaf->values.erase(leaf->values.begin()+i);
            return true;
        }
    }
    return false;
}

bool BPlusTreeIndex::remove(int key) {
    if (!root_) return false;
    Node* leaf = findLeaf(key);
    if (!removeFromLeaf(leaf, key)) return false;
    --count_;
    if (leaf == root_) {
        if (root_->keys.empty()) { freeNode(root_); root_ = nullptr; }
        return true;
    }
    if ((int)leaf->keys.size() < min_keys_) {
        Node* par = findParent(root_, leaf);
        int idx = findChildIndex(par, leaf);
        redistributeOrMergeLeaf(leaf, par, idx);
    }
    return true;
}

void BPlusTreeIndex::redistributeOrMergeLeaf(Node* node, Node* parent, int idx) {
    if (idx > 0) {
        Node* left = parent->children[idx-1];
        if ((int)left->keys.size() > min_keys_) {
            node->keys.insert(node->keys.begin(), left->keys.back());
            node->values.insert(node->values.begin(), left->values.back());
            left->keys.pop_back(); left->values.pop_back();
            parent->keys[idx-1] = node->keys.front(); return;
        }
    }
    if (idx < (int)parent->children.size()-1) {
        Node* right = parent->children[idx+1];
        if ((int)right->keys.size() > min_keys_) {
            node->keys.push_back(right->keys.front());
            node->values.push_back(right->values.front());
            right->keys.erase(right->keys.begin());
            right->values.erase(right->values.begin());
            parent->keys[idx] = right->keys.front(); return;
        }
    }
    if (idx > 0) {
        Node* left = parent->children[idx-1];
        left->keys.insert(left->keys.end(), node->keys.begin(), node->keys.end());
        left->values.insert(left->values.end(), node->values.begin(), node->values.end());
        left->next = node->next;
        parent->keys.erase(parent->keys.begin()+idx-1);
        parent->children.erase(parent->children.begin()+idx);
        --node_count_; delete node;
    } else {
        Node* right = parent->children[idx+1];
        node->keys.insert(node->keys.end(), right->keys.begin(), right->keys.end());
        node->values.insert(node->values.end(), right->values.begin(), right->values.end());
        node->next = right->next;
        parent->keys.erase(parent->keys.begin()+idx);
        parent->children.erase(parent->children.begin()+idx+1);
        --node_count_; delete right;
    }
    if (parent == root_ && parent->keys.empty()) {
        root_ = parent->children[0];
        parent->children.clear(); --node_count_; delete parent;
    } else if (parent != root_ && (int)parent->keys.size() < min_keys_) {
        Node* gp = findParent(root_, parent);
        redistributeOrMergeInternal(parent, gp, findChildIndex(gp, parent));
    }
}

void BPlusTreeIndex::redistributeOrMergeInternal(Node* node, Node* parent, int idx) {
    if (idx > 0) {
        Node* left = parent->children[idx-1];
        if ((int)left->keys.size() > min_keys_) {
            node->keys.insert(node->keys.begin(), parent->keys[idx-1]);
            node->children.insert(node->children.begin(), left->children.back());
            parent->keys[idx-1] = left->keys.back();
            left->keys.pop_back(); left->children.pop_back(); return;
        }
    }
    if (idx < (int)parent->children.size()-1) {
        Node* right = parent->children[idx+1];
        if ((int)right->keys.size() > min_keys_) {
            node->keys.push_back(parent->keys[idx]);
            node->children.push_back(right->children.front());
            parent->keys[idx] = right->keys.front();
            right->keys.erase(right->keys.begin());
            right->children.erase(right->children.begin()); return;
        }
    }
    if (idx > 0) {
        Node* left = parent->children[idx-1];
        left->keys.push_back(parent->keys[idx-1]);
        left->keys.insert(left->keys.end(), node->keys.begin(), node->keys.end());
        left->children.insert(left->children.end(), node->children.begin(), node->children.end());
        parent->keys.erase(parent->keys.begin()+idx-1);
        parent->children.erase(parent->children.begin()+idx);
        node->children.clear(); --node_count_; delete node;
    } else {
        Node* right = parent->children[idx+1];
        node->keys.push_back(parent->keys[idx]);
        node->keys.insert(node->keys.end(), right->keys.begin(), right->keys.end());
        node->children.insert(node->children.end(), right->children.begin(), right->children.end());
        parent->keys.erase(parent->keys.begin()+idx);
        parent->children.erase(parent->children.begin()+idx+1);
        right->children.clear(); --node_count_; delete right;
    }
    if (parent == root_ && parent->keys.empty()) {
        root_ = parent->children[0];
        parent->children.clear(); --node_count_; delete parent;
    } else if (parent != root_ && (int)parent->keys.size() < min_keys_) {
        Node* gp = findParent(root_, parent);
        redistributeOrMergeInternal(parent, gp, findChildIndex(gp, parent));
    }
}

BPlusTreeIndex::Node* BPlusTreeIndex::findParent(Node* cur, Node* child) const {
    if (!cur || cur->is_leaf) return nullptr;
    for (auto* c : cur->children) {
        if (c == child) return cur;
        Node* p = findParent(c, child);
        if (p) return p;
    }
    return nullptr;
}

int BPlusTreeIndex::findChildIndex(Node* parent, Node* child) const {
    for (int i = 0; i < (int)parent->children.size(); ++i)
        if (parent->children[i] == child) return i;
    return -1;
}

size_t BPlusTreeIndex::size() const { return count_; }

size_t BPlusTreeIndex::nodeMemory(Node* n) const {
    if (!n) return 0;
    size_t mem = sizeof(Node);
    mem += n->keys.capacity() * sizeof(int);
    mem += n->values.capacity() * sizeof(std::string);
    for (auto& v : n->values) mem += v.capacity();
    mem += n->children.capacity() * sizeof(Node*);
    if (!n->is_leaf) for (auto* c : n->children) mem += nodeMemory(c);
    return mem;
}

size_t BPlusTreeIndex::memoryUsage() const {
    return sizeof(*this) + nodeMemory(root_);
}

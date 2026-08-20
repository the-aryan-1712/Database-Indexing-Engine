#include "../include/btree_index.h"
#include <algorithm>

// ── Construction / Destruction ─────────────────────────────────────────
BTreeIndex::BTreeIndex(int min_degree) : t_(min_degree) {
    root_ = makeNode(true);
}

BTreeIndex::~BTreeIndex() { freeNode(root_); }

BTreeIndex::Node* BTreeIndex::makeNode(bool leaf) {
    auto* n = new Node();
    n->is_leaf = leaf;
    ++node_count_;
    return n;
}

void BTreeIndex::freeNode(Node* n) {
    if (!n) return;
    for (auto* c : n->children) freeNode(c);
    --node_count_;
    delete n;
}

// ── Search ─────────────────────────────────────────────────────────────
std::optional<std::string> BTreeIndex::search(int key) const {
    return searchNode(root_, key);
}

std::optional<std::string> BTreeIndex::searchNode(Node* n, int key) const {
    if (!n) return std::nullopt;
    int i = 0;
    while (i < (int)n->keys.size() && key > n->keys[i]) ++i;
    if (i < (int)n->keys.size() && n->keys[i] == key)
        return n->values[i];
    if (n->is_leaf) return std::nullopt;
    return searchNode(n->children[i], key);
}

// ── Insert ─────────────────────────────────────────────────────────────
void BTreeIndex::insert(int key, const std::string& value) {
    // Try to update existing
    // (Walk down to find – simpler than maintaining during split)
    {
        Node* n = root_;
        while (n) {
            int i = 0;
            while (i < (int)n->keys.size() && key > n->keys[i]) ++i;
            if (i < (int)n->keys.size() && n->keys[i] == key) {
                n->values[i] = value;
                return;
            }
            if (n->is_leaf) break;
            n = n->children[i];
        }
    }

    // Root is full - grow tree height
    if ((int)root_->keys.size() == 2 * t_ - 1) {
        Node* s = makeNode(false);
        s->children.push_back(root_);
        root_ = s;
        splitChild(s, 0);
    }
    insertNonFull(root_, key, value);
    ++count_;
}

void BTreeIndex::splitChild(Node* parent, int idx) {
    Node* y = parent->children[idx];
    Node* z = makeNode(y->is_leaf);

    int mid = t_ - 1;

    // z gets the upper half of y's keys/values
    z->keys.assign(y->keys.begin() + mid + 1, y->keys.end());
    z->values.assign(y->values.begin() + mid + 1, y->values.end());

    if (!y->is_leaf) {
        z->children.assign(y->children.begin() + t_, y->children.end());
        y->children.resize(t_);
    }

    // Push median up into parent
    parent->keys.insert(parent->keys.begin() + idx, y->keys[mid]);
    parent->values.insert(parent->values.begin() + idx, y->values[mid]);
    parent->children.insert(parent->children.begin() + idx + 1, z);

    y->keys.resize(mid);
    y->values.resize(mid);
}

void BTreeIndex::insertNonFull(Node* n, int key, const std::string& val) {
    int i = (int)n->keys.size() - 1;

    if (n->is_leaf) {
        // Insert into sorted position
        n->keys.push_back(0);
        n->values.push_back("");
        while (i >= 0 && key < n->keys[i]) {
            n->keys[i + 1] = n->keys[i];
            n->values[i + 1] = n->values[i];
            --i;
        }
        n->keys[i + 1] = key;
        n->values[i + 1] = val;
    } else {
        while (i >= 0 && key < n->keys[i]) --i;
        ++i;
        if ((int)n->children[i]->keys.size() == 2 * t_ - 1) {
            splitChild(n, i);
            if (key > n->keys[i]) ++i;
        }
        insertNonFull(n->children[i], key, val);
    }
}

// ── Delete (CLRS proactive approach) ───────────────────────────────────
bool BTreeIndex::remove(int key) {
    if (!root_ || root_->keys.empty()) return false;
    bool found = removeFromNode(root_, key);

    // Shrink tree if root is empty internal node
    if (root_->keys.empty() && !root_->is_leaf) {
        Node* old = root_;
        root_ = root_->children[0];
        old->children.clear();
        --node_count_;
        delete old;
    }
    if (found) --count_;
    return found;
}

bool BTreeIndex::removeFromNode(Node* n, int key) {
    int idx = 0;
    while (idx < (int)n->keys.size() && n->keys[idx] < key) ++idx;

    if (idx < (int)n->keys.size() && n->keys[idx] == key) {
        if (n->is_leaf) {
            removeFromLeaf(n, idx);
            return true;
        } else {
            removeFromInternal(n, idx);
            return true;
        }
    } else {
        if (n->is_leaf) return false;   // key not found
        // Ensure child has >= t keys before descending
        if ((int)n->children[idx]->keys.size() < t_)
            fill(n, idx);
        // After fill, idx might have changed if merge happened
        if (idx > (int)n->keys.size())
            return removeFromNode(n->children[idx - 1], key);
        return removeFromNode(n->children[idx], key);
    }
}

void BTreeIndex::removeFromLeaf(Node* n, int idx) {
    n->keys.erase(n->keys.begin() + idx);
    n->values.erase(n->values.begin() + idx);
}

void BTreeIndex::removeFromInternal(Node* n, int idx) {
    int key = n->keys[idx];
    if ((int)n->children[idx]->keys.size() >= t_) {
        // Replace with predecessor
        std::string val;
        int pred = getPredecessorKey(n->children[idx], val);
        n->keys[idx] = pred;
        n->values[idx] = val;
        removeFromNode(n->children[idx], pred);
    } else if ((int)n->children[idx + 1]->keys.size() >= t_) {
        // Replace with successor
        std::string val;
        int succ = getSuccessorKey(n->children[idx + 1], val);
        n->keys[idx] = succ;
        n->values[idx] = val;
        removeFromNode(n->children[idx + 1], succ);
    } else {
        // Merge children[idx] and children[idx+1]
        merge(n, idx);
        removeFromNode(n->children[idx], key);
    }
}

int BTreeIndex::getPredecessorKey(Node* n, std::string& val) {
    while (!n->is_leaf) n = n->children.back();
    val = n->values.back();
    return n->keys.back();
}

int BTreeIndex::getSuccessorKey(Node* n, std::string& val) {
    while (!n->is_leaf) n = n->children.front();
    val = n->values.front();
    return n->keys.front();
}

void BTreeIndex::merge(Node* parent, int idx) {
    Node* left  = parent->children[idx];
    Node* right = parent->children[idx + 1];

    // Pull key from parent into left
    left->keys.push_back(parent->keys[idx]);
    left->values.push_back(parent->values[idx]);

    // Append right's keys/values/children to left
    left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
    left->values.insert(left->values.end(), right->values.begin(), right->values.end());
    if (!left->is_leaf)
        left->children.insert(left->children.end(), right->children.begin(), right->children.end());

    // Remove key/child from parent
    parent->keys.erase(parent->keys.begin() + idx);
    parent->values.erase(parent->values.begin() + idx);
    parent->children.erase(parent->children.begin() + idx + 1);

    right->children.clear();
    --node_count_;
    delete right;
}

void BTreeIndex::fill(Node* parent, int idx) {
    if (idx > 0 && (int)parent->children[idx - 1]->keys.size() >= t_)
        borrowFromPrev(parent, idx);
    else if (idx < (int)parent->children.size() - 1 &&
             (int)parent->children[idx + 1]->keys.size() >= t_)
        borrowFromNext(parent, idx);
    else {
        if (idx < (int)parent->children.size() - 1)
            merge(parent, idx);
        else
            merge(parent, idx - 1);
    }
}

void BTreeIndex::borrowFromPrev(Node* parent, int idx) {
    Node* child = parent->children[idx];
    Node* sibling = parent->children[idx - 1];

    child->keys.insert(child->keys.begin(), parent->keys[idx - 1]);
    child->values.insert(child->values.begin(), parent->values[idx - 1]);

    parent->keys[idx - 1] = sibling->keys.back();
    parent->values[idx - 1] = sibling->values.back();

    if (!child->is_leaf) {
        child->children.insert(child->children.begin(), sibling->children.back());
        sibling->children.pop_back();
    }
    sibling->keys.pop_back();
    sibling->values.pop_back();
}

void BTreeIndex::borrowFromNext(Node* parent, int idx) {
    Node* child = parent->children[idx];
    Node* sibling = parent->children[idx + 1];

    child->keys.push_back(parent->keys[idx]);
    child->values.push_back(parent->values[idx]);

    parent->keys[idx] = sibling->keys.front();
    parent->values[idx] = sibling->values.front();

    if (!child->is_leaf) {
        child->children.push_back(sibling->children.front());
        sibling->children.erase(sibling->children.begin());
    }
    sibling->keys.erase(sibling->keys.begin());
    sibling->values.erase(sibling->values.begin());
}

// ── Misc ───────────────────────────────────────────────────────────────
size_t BTreeIndex::size() const { return count_; }

size_t BTreeIndex::nodeMemory(Node* n) const {
    if (!n) return 0;
    size_t mem = sizeof(Node);
    mem += n->keys.capacity() * sizeof(int);
    mem += n->values.capacity() * sizeof(std::string);
    for (auto& v : n->values) mem += v.capacity();
    mem += n->children.capacity() * sizeof(Node*);
    for (auto* c : n->children) mem += nodeMemory(c);
    return mem;
}

size_t BTreeIndex::memoryUsage() const {
    return sizeof(*this) + nodeMemory(root_);
}

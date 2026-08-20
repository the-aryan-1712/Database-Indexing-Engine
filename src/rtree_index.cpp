#include "../include/rtree_index.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include <cassert>

RTreeIndex::RTreeIndex(int max_entries, int min_entries)
    : max_entries_(max_entries), min_entries_(min_entries), root_(nullptr) {}

RTreeIndex::~RTreeIndex() { freeNode(root_); }

RTreeIndex::Node* RTreeIndex::makeNode(bool leaf) {
    auto* n = new Node(); n->is_leaf = leaf; ++node_count_; return n;
}

void RTreeIndex::freeNode(Node* n) {
    if (!n) return;
    if (!n->is_leaf)
        for (auto& e : n->entries) freeNode(e.child);
    --node_count_; delete n;
}

Point2D RTreeIndex::keyToPoint(int key) const {
    return {static_cast<double>(key), static_cast<double>((key * 37) % 10007)};
}

MBR RTreeIndex::computeMBR(Node* node) const {
    if (node->entries.empty()) return {};
    MBR m = node->entries[0].mbr;
    for (size_t i = 1; i < node->entries.size(); ++i) m.expand(node->entries[i].mbr);
    return m;
}

// ── IIndex wrappers ────────────────────────────────────────────────────
void RTreeIndex::insert(int key, const std::string& value) {
    Point2D pt = keyToPoint(key);
    removePoint(pt);  // ensure unique keys
    insertPoint(pt, value);
}

std::optional<std::string> RTreeIndex::search(int key) const {
    Point2D pt = keyToPoint(key);
    MBR q = MBR::from_point(pt);
    auto results = rangeQuery(q);
    for (auto& [p, v] : results)
        if (p == pt) return v;
    return std::nullopt;
}

bool RTreeIndex::remove(int key) {
    return removePoint(keyToPoint(key));
}

// ── Spatial insert ─────────────────────────────────────────────────────
void RTreeIndex::insertPoint(const Point2D& pt, const std::string& value) {
    Entry entry;
    entry.mbr = MBR::from_point(pt);
    entry.value = value;
    entry.child = nullptr;

    if (!root_) {
        root_ = makeNode(true);
        root_->entries.push_back(entry);
        ++count_; return;
    }

    insertEntry(root_, entry, nullptr);
    ++count_;
}

void RTreeIndex::insertEntry(Node* node, const Entry& entry, Node* parent) {
    if (node->is_leaf) {
        node->entries.push_back(entry);
        if ((int)node->entries.size() > max_entries_)
            splitNode(node, parent);
        return;
    }
    // Choose child with least enlargement
    int best = 0;
    double bestEnl = std::numeric_limits<double>::max();
    for (int i = 0; i < (int)node->entries.size(); ++i) {
        double enl = node->entries[i].mbr.enlargement(entry.mbr);
        if (enl < bestEnl || (enl == bestEnl && node->entries[i].mbr.area() <
                              node->entries[best].mbr.area())) {
            bestEnl = enl; best = i;
        }
    }
    node->entries[best].mbr.expand(entry.mbr);
    insertEntry(node->entries[best].child, entry, node);
    // Update MBR after potential child split
    if (best < (int)node->entries.size())
        node->entries[best].mbr = computeMBR(node->entries[best].child);
}

// ── Quadratic split ────────────────────────────────────────────────────
std::pair<int,int> RTreeIndex::pickSeeds(const std::vector<Entry>& entries) {
    int s1 = 0, s2 = 1;
    double worst = -std::numeric_limits<double>::max();
    for (int i = 0; i < (int)entries.size(); ++i) {
        for (int j = i + 1; j < (int)entries.size(); ++j) {
            MBR combined = entries[i].mbr;
            combined.expand(entries[j].mbr);
            double waste = combined.area() - entries[i].mbr.area() - entries[j].mbr.area();
            if (waste > worst) { worst = waste; s1 = i; s2 = j; }
        }
    }
    return {s1, s2};
}

void RTreeIndex::splitNode(Node* node, Node* parent) {
    std::vector<Entry> all = std::move(node->entries);
    node->entries.clear();

    auto [s1, s2] = pickSeeds(all);
    Node* newNode = makeNode(node->is_leaf);

    node->entries.push_back(all[s1]);
    newNode->entries.push_back(all[s2]);

    std::vector<bool> assigned(all.size(), false);
    assigned[s1] = assigned[s2] = true;
    int unassigned = (int)all.size() - 2;

    while (unassigned > 0) {
        // Check min fill constraints
        if ((int)node->entries.size() + unassigned <= min_entries_) {
            for (size_t i = 0; i < all.size(); ++i)
                if (!assigned[i]) { node->entries.push_back(all[i]); assigned[i] = true; }
            break;
        }
        if ((int)newNode->entries.size() + unassigned <= min_entries_) {
            for (size_t i = 0; i < all.size(); ++i)
                if (!assigned[i]) { newNode->entries.push_back(all[i]); assigned[i] = true; }
            break;
        }

        // Pick next: entry with max |d1 - d2|
        MBR mbr1 = computeMBR(node), mbr2 = computeMBR(newNode);
        int pick = -1; double maxDiff = -1;
        for (size_t i = 0; i < all.size(); ++i) {
            if (assigned[i]) continue;
            double d1 = mbr1.enlargement(all[i].mbr);
            double d2 = mbr2.enlargement(all[i].mbr);
            double diff = std::fabs(d1 - d2);
            if (diff > maxDiff || pick < 0) { maxDiff = diff; pick = (int)i; }
        }

        double e1 = computeMBR(node).enlargement(all[pick].mbr);
        double e2 = computeMBR(newNode).enlargement(all[pick].mbr);
        if (e1 < e2 || (e1 == e2 && computeMBR(node).area() <= computeMBR(newNode).area())) {
            node->entries.push_back(all[pick]);
        } else {
            newNode->entries.push_back(all[pick]);
        }
        assigned[pick] = true;
        --unassigned;
    }

    // Insert newNode into parent
    Entry newEntry;
    newEntry.mbr = computeMBR(newNode);
    newEntry.child = newNode;

    if (!parent) {
        // node was root - create new root
        Node* newRoot = makeNode(false);
        Entry e1; e1.mbr = computeMBR(node); e1.child = node;
        newRoot->entries.push_back(e1);
        newRoot->entries.push_back(newEntry);
        root_ = newRoot;
    } else {
        // Update parent's MBR for the split node
        for (auto& pe : parent->entries)
            if (pe.child == node) { pe.mbr = computeMBR(node); break; }
        parent->entries.push_back(newEntry);
        if ((int)parent->entries.size() > max_entries_) {
            Node* gp = findParent(root_, parent);
            splitNode(parent, gp);
        }
    }
}

// ── Range query ────────────────────────────────────────────────────────
std::vector<std::pair<Point2D,std::string>> RTreeIndex::rangeQuery(const MBR& query) const {
    std::vector<std::pair<Point2D,std::string>> results;
    if (root_) rangeSearch(root_, query, results);
    return results;
}

void RTreeIndex::rangeSearch(Node* node, const MBR& query,
                              std::vector<std::pair<Point2D,std::string>>& results) const {
    for (auto& e : node->entries) {
        if (!e.mbr.intersects(query)) continue;
        if (node->is_leaf) {
            Point2D pt{e.mbr.x_min, e.mbr.y_min};
            if (query.contains(pt)) results.emplace_back(pt, e.value);
        } else {
            rangeSearch(e.child, query, results);
        }
    }
}

// ── Remove ─────────────────────────────────────────────────────────────
// Collect all leaf entries from a subtree
static void collectLeafEntries(RTreeIndex::Node* n,
                               std::vector<std::pair<Point2D,std::string>>& out) {
    if (n->is_leaf) {
        for (auto& e : n->entries)
            out.emplace_back(Point2D{e.mbr.x_min, e.mbr.y_min}, e.value);
    } else {
        for (auto& e : n->entries)
            collectLeafEntries(e.child, out);
    }
}

bool RTreeIndex::removePoint(const Point2D& pt) {
    if (!root_) return false;
    if (removeEntry(root_, pt, nullptr)) { --count_; return true; }
    return false;
}

bool RTreeIndex::removeEntry(Node* node, const Point2D& pt, Node* parent) {
    if (node->is_leaf) {
        for (auto it = node->entries.begin(); it != node->entries.end(); ++it) {
            Point2D ep{it->mbr.x_min, it->mbr.y_min};
            if (ep == pt) {
                node->entries.erase(it);
                // Handle root becoming empty
                if (node == root_ && node->entries.empty()) {
                    --node_count_; delete root_; root_ = nullptr;
                }
                return true;
            }
        }
        return false;
    }

    // Internal node: try each child whose MBR contains the point
    for (int ci = 0; ci < (int)node->entries.size(); ++ci) {
        if (!node->entries[ci].mbr.contains(pt)) continue;
        if (removeEntry(node->entries[ci].child, pt, node)) {
            // Child may be empty or underflowed
            Node* child = node->entries[ci].child;

            if (child && child->entries.empty()) {
                // Remove empty child
                --node_count_; delete child;
                node->entries.erase(node->entries.begin() + ci);
            } else if (child && (int)child->entries.size() < min_entries_) {
                // Underflow: remove child, collect its entries, reinsert
                std::vector<std::pair<Point2D,std::string>> orphans;
                collectLeafEntries(child, orphans);
                freeNode(child);
                node->entries.erase(node->entries.begin() + ci);
                // Reinsert orphans
                for (auto& [p, v] : orphans) {
                    --count_;  // will be re-incremented by insertPoint
                    insertPoint(p, v);
                }
            } else if (child) {
                // Update MBR
                node->entries[ci].mbr = computeMBR(child);
            }

            // Shrink root if it's internal with one child
            if (node == root_ && !root_->is_leaf && (int)root_->entries.size() == 1) {
                Node* old = root_;
                root_ = root_->entries[0].child;
                old->entries.clear();
                --node_count_; delete old;
            }
            // Root became empty
            if (node == root_ && root_->entries.empty()) {
                --node_count_; delete root_; root_ = nullptr;
            }
            return true;
        }
    }
    return false;
}

void RTreeIndex::condenseTree(Node*, Node*) {
    // Handled inline in removeEntry
}

RTreeIndex::Node* RTreeIndex::findParent(Node* cur, Node* child) const {
    if (!cur || cur->is_leaf) return nullptr;
    for (auto& e : cur->entries) {
        if (e.child == child) return cur;
        Node* p = findParent(e.child, child);
        if (p) return p;
    }
    return nullptr;
}

size_t RTreeIndex::size() const { return count_; }

size_t RTreeIndex::nodeMemory(Node* n) const {
    if (!n) return 0;
    size_t mem = sizeof(Node);
    mem += n->entries.capacity() * sizeof(Entry);
    for (auto& e : n->entries) {
        mem += e.value.capacity();
        if (e.child) mem += nodeMemory(e.child);
    }
    return mem;
}

size_t RTreeIndex::memoryUsage() const {
    return sizeof(*this) + (root_ ? nodeMemory(root_) : 0);
}

#include "../include/bplus_tree_index.h"
#include <iostream>

int main() {
    BPlusTreeIndex tree(6);

    for (int i = 0; i < 100000; i++) {
        // std::cout << "Inserting: " << i << "\n";
        tree.insert(i, "v" + std::to_string(i));
    }
    std::cout<<"done"<<"\n";

    std::cout << "Done\n";
}
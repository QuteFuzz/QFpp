#ifndef AST_UTILS_H
#define AST_UTILS_H

#include <vector>
#include <memory>
#include <node.h>

using Slot_type = std::shared_ptr<Node>*;

/// Node generator class that works like python's iterator to handle looping through the AST
class Node_gen {

    public:
        class Iterator {
            public:
                Iterator(Node& root, Token_kind kind, std::vector<Slot_type>& visited)
                    : _root(&root), _kind(kind), _visited(&visited) {
                    // Prime the first element
                    _current_slot = _root->find_slot(_kind, *_visited);
                }

                // Constructor for the "end" iterator
                Iterator() : _current_slot(nullptr), _root(nullptr), _visited(nullptr) {}

                // Dereference operator: returns the slot (allow modification like swap)
                std::shared_ptr<Node>& operator*() {
                    return *_current_slot;
                }

                // Arrow operator
                Slot_type operator->() {
                    return _current_slot;
                }

                // Post-increment (it++): Stores current iterator to return it later, then uses ++it to modify in-place
                Iterator operator++(int) {
                    Iterator it = *this;
                    ++(*this);
                    return it;
                }

                // Pre-increment (++it): Finds the next slot
                Iterator& operator++() {
                    if (_root) {
                        _current_slot = _root->find_slot(_kind, *_visited);
                    }
                    return *this;
                }

                // Inequality check
                bool operator!=(const Iterator& other) const {
                    return _current_slot != other._current_slot;
                }

            private:
                Slot_type _current_slot;
                Node* _root;
                Token_kind _kind;
                std::vector<Slot_type>* _visited;
        };

        Node_gen(Node& root, Token_kind kind) :
            root(root),
            kind(kind)
        {}

        Iterator begin() {return Iterator(root, kind, visited_slots);}

        Iterator end() {return Iterator();}

    private:
        Node& root;
        Token_kind kind;
        std::vector<Slot_type> visited_slots;
};

#endif

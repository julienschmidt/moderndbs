#ifndef BTREE_H_
#define BTREE_H_

#include "Segment.hpp"
#include "TID.hpp"

// DEBUG
#include <iostream>

template <class K, class LESS = std::less<K>>
class BTree : public Segment {
    // index used to indicate that a key was not found
    static const unsigned keyNotFound = -1;

    class Node {
      protected:
        // LSN for recovery
        bool     leaf;  // node is a leaf; TODO: borrow a bit somewhere?
        unsigned count; // number of entries

        Node(bool leaf) : leaf(leaf), count(0) {};

      public:
        inline bool isLeaf() { return leaf; }

        // pure virtual methods
        virtual bool     isFull() = 0;
        virtual unsigned getKeyIndex(K key) = 0;
    };

    class InnerNode : public Node {
        // calculate tree order n = 2k from page size
        static const size_t order =
            (blocksize - sizeof(Node) - sizeof(uint64_t)) /
            (sizeof(K) + sizeof(uint64_t));

        // compare less function
        LESS less;

        K        keys[order];
        uint64_t children[order+1];

      public:
        InnerNode() : Node(false) {};

        inline bool isFull() {
            return this->count == order+1;
        }

        // TODO: binary search
        unsigned getKeyIndex(K key) {
            for (unsigned i = 0; i < this->count; ++i) {
                if(less(key, keys[i]))
                    return i;
            }
            return this->count; // greater than all existing keys
        }

        uint64_t getChild(K key) {
            return children[getKeyIndex(key)];
        }
    };

    class LeafNode : public Node {
        // calculate tree order n = 2k from page size
        static const size_t order =
            (blocksize - sizeof(Node)) /
            (sizeof(K) + sizeof(TID));

        // compare less function
        LESS less;

        // TODO: next / prev leaf node

        K   keys[order];
        TID tids[order];

      public:
        LeafNode() : Node(true) {};

        inline bool isFull() {
            return this->count == order;
        }

        // TODO: binary search
        // returns index of first existing key >= input key
        unsigned getKeyIndex(K key) {
            for (unsigned i = 0; i < this->count; ++i) {
                if (!less(keys[i], key)) { // TODO: check == ?
                    return i;
                }
            }
            return keyNotFound;
        }

        bool getTID(K key, TID& tid) {
            unsigned i = getKeyIndex(key);

            if(i == keyNotFound || less(key, keys[i]))
                return false;

            tid = tids[i];
            return true;
        }

        void insert(K key, TID tid) {
            std::cout << " LeafNode::insert " << std::endl;
            unsigned i = getKeyIndex(key);
            if (i == keyNotFound) {
                i = this->count; // greater than all existing keys
            } else if (!less(key, keys[i])) { // existing key? (checks other direction)
                tids[i] = tid; // overwrite existing value
                return;
            } else {
                // move existing entries
                memmove(keys+i+1, keys+i, (this->count-i) * sizeof(K));
                memmove(tids+i+1, tids+i, (this->count-i) * sizeof(TID));
            }

            assert(!isFull());

            // insert new entry
            keys[i] = key;
            tids[i] = tid;
            this->count++;

            std::cout << "new leaf size " << this->count << std::endl;
        }

        // returns false if the key was not found
        bool remove(K key) {
            unsigned i = getKeyIndex(key);
            if (i == keyNotFound)
                return false;

            // move remaining entries
            memmove(keys+i, keys+i+1, (this->count-i-1) * sizeof(K));
            memmove(tids+i, tids+i+1, (this->count-i-1) * sizeof(TID));
            this->count--;
            return true;
        }
    };

  public:
    BTree(BufferManager& bm, uint64_t id) : Segment(bm, id), root(0) {
        // TODO: pageID  | (&SegmentID + shiften n stuff)

        std::cout << "LeafNode::order" << ((blocksize - sizeof(Node)) / (sizeof(K) + sizeof(TID))) << std::endl;

        // init root node
        BufferFrame&  bf      = bm.fixPage(root, true);
        void*         dataPtr = bf.getData();
        new (dataPtr) LeafNode();
        bm.unfixPage(bf, true);
        size = 1;
    };


    /*
    Lookup:
    1. start with the root node
    2. is the current node a leaf?
        > if yes, return the current page (locate the entry on it)
    3. find the first entry ≥ search key (binary search)
    4. if no such entry is found, go the upper, otherwise go to the corresponding page
    5. continue with 2
    */
    bool lookup(K key, TID& tid) {
        LeafNode*    leaf;
        BufferFrame& bf = findLeaf(key, false, &leaf);

        bool found = leaf->getTID(key, tid);
        bm.unfixPage(bf, false);
        return found;
    }


    /*
    Insert:
    1. lookup the appropriate leaf page
    2. is there free space on the leaf
        > if yes, insert entry and stop
    3. split the leaf into two, insert entry on proper side
    4. insert maximum of left page as separator into parent
    5. if the parent overflow, split parent and continue with 4
    6. create a new root if needed
    */
    void insert(K key, TID tid) {
        BufferFrame& bf      = bm.fixPage(root, true);
        Node*        current = static_cast<Node*>(bf.getData());

        assert(!current->isFull());
        if (!current->isFull()) {
            assert(current->isLeaf());
            if (current->isLeaf()) {
                // found the leaf where the entry must be inserted
                LeafNode* leaf = reinterpret_cast<LeafNode*>(current);
                leaf->insert(key, tid);

                bm.unfixPage(bf, true);
            } else {
                // TODO: traverse
            }
        } else {
            // TODO: split node
        }
    };


    /*
    TODO: Delete
    1. lookup the appropriate leaf page
    2. remove the entry from the current page
    3. is the current page at least half full?
        > if yes, stop
    4. is the neighboring page more than half full?
        > if yes, balance both pages, update separator, and stop
    5. merge neighboring page into current page
    6. remove the separator from the parent, continue with 3
    */
    bool erase(K key) {
        LeafNode*    leaf;
        BufferFrame& bf   = findLeaf(key, true, &leaf);

        bool found = leaf->remove(key);
        bm.unfixPage(bf, found);
        // TODO: re-balancing
        return found;
    };

  private:
    uint64_t root;

    // Finds the leaf for the given key
    // Returns the respective BufferFrame containing the Leaf
    BufferFrame& findLeaf(K& key, bool exclusive, LeafNode** leafPtr) {
        std::cout << "BTree::findLeaf " << std::endl;

        // get root node
        BufferFrame& bf     = bm.fixPage(root, exclusive);
        Node*        node   = static_cast<Node*>(bf.getData());

        // find leaf
        while(!node->isLeaf()) {
            std::cout << "getChild " << std::endl;
            InnerNode* inner  = reinterpret_cast<InnerNode*>(node);
            uint64_t   nextID = inner->getChild(key);
            bm.unfixPage(bf, false);

            // get next node
            bf   = bm.fixPage(nextID, exclusive);
            node = static_cast<Node*>(bf.getData());
        }

        assert(node->isLeaf());
        *leafPtr = reinterpret_cast<LeafNode*>(node);
        return bf;
    }
};

#endif  // BTREE_H_


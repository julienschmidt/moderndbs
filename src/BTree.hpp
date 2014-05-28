#ifndef BTREE_H_
#define BTREE_H_

#include <algorithm>

#include "Segment.hpp"
#include "TID.hpp"

// DEBUG
#include <iostream>

template <class K, class LESS = std::less<K>>
class BTree : public Segment {
    // compare less function
    LESS less;

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

        K        keys[order];
        uint64_t children[order+1];

        InnerNode() : Node(false) {};

      public:
        InnerNode(uint64_t leftChild, uint64_t rightChild, K sep) : Node(false) {
            keys[0]     = sep;
            children[0] = leftChild;
            children[1] = rightChild;
            this->count = 2;
        };

        inline bool isFull() {
            return this->count == order+1;
        }

        inline K maxKey() {
            return keys[this->count-2];
        }

        unsigned getKeyIndex(K key) {
            // compare less function
            LESS less;
            return std::lower_bound(keys, keys+this->count-1, key, less)-keys;
        }

        uint64_t getChild(K key) {
            return children[getKeyIndex(key)];
        }

        void insert(K key, uint64_t child) {
            //std::cout << "InnerNode::insert key=" << key << " childID=" << child << std::endl;
            unsigned i = getKeyIndex(key);
            //std::cout << " i=" << i << " count=" << this->count << std::endl;

            if (i < this->count-1) {
                assert(false);
                /*if (!less(key, keys[i])) { // existing key? (checks other direction)
                    children[i] = child; // overwrite existing value
                    std::cout << "  overwritten i=" << i << std::endl;
                    return;
                } else {*/
                // move existing entries
                memmove(keys+i+1, keys+i, (this->count-i-1) * sizeof(K));
                memmove(children+i+2, children+i+1, (this->count-i-1) * sizeof(uint64_t));
                /*}*/
            }

            assert(!isFull());

            // insert new entry
            keys[i] = key;
            children[i+1] = child;
            this->count++;
        }

        // returns the separator key
        K split(BufferFrame& bf) {
            assert(isFull());

            InnerNode* newInner = new (bf.getData()) InnerNode();

            unsigned middle  = this->count / 2;
            this->count     -= middle;
            newInner->count  = middle;

            memcpy(newInner->keys,     keys+this->count-1,  (middle-1) * sizeof(K));
            memcpy(newInner->children, children+this->count, middle * sizeof(uint64_t));

            // return separator key
            return keys[this->count-1];
        }

        void print() {
            /*std::cout << "Inner order=" << order << " count=" << this->count << " min=" << keys[0] << " max=" << maxKey() << std::endl;
            for(unsigned i=0; i < this->count; ++i) {
                std::cout << "   " << i << ": " << keys[i] << " = " << children[i] << std::endl;
            }
            std::cout << std::endl;*/
        }
    };

    class LeafNode : public Node {
        // calculate tree order n = 2k from page size
        static const size_t order =
            (blocksize - sizeof(Node)) /
            (sizeof(K) + sizeof(TID));

        // TODO: next / prev leaf node

        K   keys[order];
        TID tids[order];

      public:


        LeafNode() : Node(true) {};

        inline bool isFull() {
            return this->count == order;
        }

        inline K maxKey() {
            return keys[this->count-1];
        }

        // returns index of first existing key >= input key
        unsigned getKeyIndex(K key) {
            // compare less function
            LESS less;
            return std::lower_bound(keys, keys+this->count, key, less)-keys;
        }

        bool getTID(K key, TID& tid) {
            unsigned i = getKeyIndex(key);

            // compare less function
            LESS less;

            if(i == this->count || less(key, keys[i]))
                return false;

            tid = tids[i];
            return true;
        }

        void insert(K key, TID tid) {
            unsigned i = getKeyIndex(key);
            if (i < this->count) {
                // compare less function
                LESS less;
                if (!less(key, keys[i])) { // existing key? (checks other direction)
                    tids[i] = tid; // overwrite existing value
                    return;
                } else {
                    // move existing entries
                    memmove(keys+i+1, keys+i, (this->count-i) * sizeof(K));
                    memmove(tids+i+1, tids+i, (this->count-i) * sizeof(TID));
                }
            }

            assert(!isFull());

            // insert new entry
            keys[i] = key;
            tids[i] = tid;
            this->count++;
        }

        // returns false if the key was not found
        bool remove(K key) {
            unsigned i = getKeyIndex(key);
            if (i == this->count)
                return false;

            // move remaining entries
            memmove(keys+i, keys+i+1, (this->count-i-1) * sizeof(K));
            memmove(tids+i, tids+i+1, (this->count-i-1) * sizeof(TID));
            this->count--;
            return true;
        }

        K split(BufferFrame& bf) {
            assert(isFull());

            LeafNode* newLeaf = new (bf.getData()) LeafNode();

            unsigned middle = this->count / 2;
            this->count    -= middle;
            newLeaf->count  = middle;

            // move keys and TIDs to the new leaf
            K*   keyStart = keys+this->count;
            TID* tidStart = tids+this->count;
            std::copy(keyStart, keyStart+middle, newLeaf->keys);
            std::copy(tidStart, tidStart+middle, newLeaf->tids);

            return maxKey();
        }

        /*void print() {
            std::cout << "Leaf order=" << order << " count=" << this->count << " min=" << keys[0] << " max=" << keys[this->count-1] << std::endl;
            //for(unsigned i=0; i < this->count; ++i) {
            //    std::cout << "   " << i << ": " << keys[i] << " = " << tids[i] << std::endl;
            //}
            //std::cout << std::endl;
        }*/
    };

  public:
    BTree(BufferManager& bm, uint64_t id) : Segment(bm, id), root(0) {
        // TODO: pageID  | (&SegmentID + shiften n stuff)

        static_assert(sizeof(LeafNode) <= blocksize, "LeafNode size exceeds page size");
        static_assert(sizeof(InnerNode) <= blocksize, "InnerNode size exceeds page size");

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
        BufferFrame* bf   = &bm.fixPage(root, true);
        Node*        node = static_cast<Node*>(bf->getData());

        // parent
        BufferFrame* bfPar    = NULL;  // root has no parent
        bool         parDirty = false; // was the page modified?

        // uses "safe" inner pages: split all full nodes on the way down
        while(true) {
            if (node->isFull()) { // must split current node
                // open a new page where the new node will be written to
                uint64_t     newID = reservePage();
                BufferFrame* bfNew = &bm.fixPage(newID, true);
                std::cout << "  split "<< bf->getID() <<"; new pageID: " << newID << " "  << node->isLeaf() << std::endl;

                K separator;
                if (node->isLeaf()) {
                    // construct new leaf and move half of the entries
                    LeafNode* oldLeaf = reinterpret_cast<LeafNode*>(node);
                    separator = oldLeaf->split(*bfNew);
                } else {
                    // construct new inner node and move half of the entries
                    InnerNode* oldInner = reinterpret_cast<InnerNode*>(node);
                    separator = oldInner->split(*bfNew);
                }

                // check if splitted node is the root
                if (bfPar == NULL) {
                    // reserve another page to move the old root to
                    uint64_t     moveID = reservePage();
                    BufferFrame* bfMove = &bm.fixPage(moveID, true);

                    // move current root node to the new page
                    memcpy(bfMove->getData(), bf->getData(), blocksize);

                    // init new root and insert old root as leftmost child
                    new (bf->getData()) InnerNode(moveID, bfNew->getID(), separator);
                    parDirty = true;
                    bfPar = bf;
                    bf    = bfMove;
                } else {
                    // update parent
                    InnerNode* parent = static_cast<InnerNode*>(bfPar->getData());
                    parent->print();
                    std::cout << "----- insert ------" << std::endl;
                    parent->insert(separator, bfNew->getID());
                    parent->print();
                    parDirty = true;
                    //bm.unfixPage(*bfPar, true);
                    //bfPar = bf;
                }

                // insert entry
                if (!less(separator, key)) { // key <= oldMax
                    bm.unfixPage(*bfNew, true);
                } else {
                    bm.unfixPage(*bf, true);
                    bf = bfNew;
                }
                node  = static_cast<Node*>(bf->getData());

            } else { // enough space
                if (node->isLeaf()) {
                    // found the leaf where the entry must be inserted
                    LeafNode* leaf = reinterpret_cast<LeafNode*>(node);
                    leaf->insert(key, tid);

                    if (bfPar != NULL) {
                        bm.unfixPage(*bfPar, parDirty);
                    }

                    bm.unfixPage(*bf, true);
                    return;
                } else {
                    // traverse without splitting
                    InnerNode* inner  = reinterpret_cast<InnerNode*>(node);
                    uint64_t   nextID = inner->getChild(key);

                    // lock coupling: fix child before releasing parent
                    BufferFrame* bfNew = &bm.fixPage(nextID, true);
                    if (bfPar != NULL) {
                        bm.unfixPage(*bfPar, parDirty);
                    }
                    bfPar    = bf;
                    bf       = bfNew;
                    parDirty = false;

                    node = static_cast<Node*>(bf->getData());
                }
            }
        }
    };


    /*
    Delete
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

    void debug() {
        BufferFrame& bf     = bm.fixPage(root, false);
        Node*        node   = static_cast<Node*>(bf.getData());

        std::cout << "root leaf=" << node->isLeaf() << std::endl;
        if(node->isLeaf()) {
            LeafNode* leaf = reinterpret_cast<LeafNode*>(node);
            leaf->print();
        } else {
            InnerNode* inner = reinterpret_cast<InnerNode*>(node);
            inner->debug(bm);
        }

        bm.unfixPage(bf, false);
    }

  private:
    uint64_t root;

    // Finds the leaf for the given key
    // Returns the respective BufferFrame containing the leaf
    BufferFrame& findLeaf(K& key, bool exclusive, LeafNode** leafPtr) {
        // get root node
        BufferFrame* bf     = &bm.fixPage(root, exclusive);
        Node*        node   = static_cast<Node*>(bf->getData());

        // find leaf
        while(!node->isLeaf()) {
            InnerNode* inner  = reinterpret_cast<InnerNode*>(node);
            uint64_t   nextID = inner->getChild(key);

            // lock coupling: fix child before releasing parent
            BufferFrame* bfNew = &bm.fixPage(nextID, exclusive);
            bm.unfixPage(*bf, false);
            bf = bfNew;

            node = static_cast<Node*>(bf->getData());
        }

        assert(node->isLeaf());
        *leafPtr = reinterpret_cast<LeafNode*>(node);
        return *bf;
    }

    uint64_t reservePage() {
        // size is atomic
        // TODO: | segmentID + shiften
        return size++;
    }
};

#endif  // BTREE_H_


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

        // compare less function
        LESS less;

        K        keys[order];
        uint64_t children[order+1];

      public:
        InnerNode() : Node(false) {};

        inline bool isFull() {
            return this->count == order+1;
        }

        inline K maxKey() {
            return keys[this->count-1];
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

        void insert(K key, uint64_t child) {
            std::cout << " InnerNode::insert " << std::endl;
            unsigned i = getKeyIndex(key);
            if (i == keyNotFound) {
                //i = this->count; // greater than all existing keys
            } else if (!less(key, keys[i])) { // existing key? (checks other direction)
                children[i] = child; // overwrite existing value
                return;
            } else {
                // move existing entries
                memmove(keys+i+1, keys+i, (this->count-i) * sizeof(K));
                memmove(children+i+1, children+i, (this->count-i+1) * sizeof(uint64_t));
            }

            assert(!isFull());

            // insert new entry
            keys[i] = key;
            children[i] = child;
            this->count++;

            std::cout << "new leaf size " << this->count << std::endl;
        }

        void split(InnerNode* newInner) {
            assert(isFull());

            unsigned middle  = this->count / 2;
            this->count     -= middle;
            newInner->count  = middle;

            memcpy(newInner->keys,     keys+middle,     middle * sizeof(K));
            memcpy(newInner->children, children+middle, middle * sizeof(uint64_t));
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

        inline K maxKey() {
            return keys[this->count-1];
        }

        // TODO: binary search
        // returns index of first existing key >= input key
        unsigned getKeyIndex(K key) {
            for (unsigned i = 0; i < this->count; ++i) {
                if (!less(keys[i], key)) {
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

        void split(LeafNode* newLeaf) {
            assert(isFull());

            unsigned middle = this->count / 2;
            this->count    -= middle;
            newLeaf->count  = middle;

            memcpy(newLeaf->keys, keys+middle, middle * sizeof(K));
            memcpy(newLeaf->tids, tids+middle, middle * sizeof(TID));
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
        BufferFrame* bf   = &bm.fixPage(root, true);
        Node*        node = static_cast<Node*>(bf->getData());

        // parent
        BufferFrame* bfPar = NULL; // root has no parent

        // uses "safe" inner pages: split all full nodes on the way down
        while(true) {
            if (node->isFull()) { // must split current node
                uint64_t     newID = reservePage();
                BufferFrame* bfNew = &bm.fixPage(newID, true);
                std::cout << "new pageID: " << newID << std::endl;

                K oldMax, newMax; // greatest key values

                // check if splitting root
                if (bfPar == NULL) {
                    LeafNode* oldRoot = reinterpret_cast<LeafNode*>(node);
                    oldMax = oldRoot->maxKey();

                    // move current root node to a new page
                    memcpy(bfNew->getData(), bf->getData(), blocksize);
                    bfPar = bf;
                    bf    = bfNew;
                    node  = static_cast<Node*>(bf->getData());
                    std::cout << "moved old root to pageID: " << newID << std::endl;

                    // init new root
                    InnerNode* newRoot = new (bfPar->getData()) InnerNode();

                    // insert old root
                    newRoot->insert(oldMax, newID);

                    // reserve another page for splitting the old root
                    newID = reservePage();
                    bfNew = &bm.fixPage(newID, true);
                    std::cout << "new pageID: " << newID << std::endl;
                }

                if (node->isLeaf()) {
                    // construct new leaf and move half of the entries
                    LeafNode* newLeaf = new (bfNew->getData()) LeafNode();
                    LeafNode* oldLeaf = reinterpret_cast<LeafNode*>(node);
                    oldLeaf->split(newLeaf);
                    oldMax = oldLeaf->maxKey();
                    newMax = newLeaf->maxKey();
                } else {
                    // construct new inner node and move half of the entries
                    InnerNode* newInner = new (bfNew->getData()) InnerNode();
                    InnerNode* oldInner = reinterpret_cast<InnerNode*>(node);
                    oldInner->split(newInner);
                    oldMax = oldInner->maxKey();
                    newMax = newInner->maxKey();
                }

                // insert / update nodes in parent
                InnerNode* parent = static_cast<InnerNode*>(bfPar->getData());
                parent->insert(oldMax, bf->getID());
                parent->insert(newMax, bfNew->getID()); // updates old entry

                bm.unfixPage(*bfPar, true);
                bfPar = bf;

                // insert entry
                if (less(oldMax, key)) {
                    bm.unfixPage(*bf, true);
                    bf = bfNew;
                    node = static_cast<Node*>(bf->getData());
                } else {
                    bm.unfixPage(*bfNew, true);
                }
            } else { // enough space
                if (node->isLeaf()) {
                    // found the leaf where the entry must be inserted
                    LeafNode* leaf = reinterpret_cast<LeafNode*>(node);
                    leaf->insert(key, tid);

                    if (bfPar != NULL) {
                        // TODO: could this page be dirty?
                        bm.unfixPage(*bfPar, false);
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
                        // TODO: could this page be dirty?
                        bm.unfixPage(*bfPar, false);
                    }
                    bfPar = bf;
                    bf = bfNew;

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
            InnerNode* inner  = reinterpret_cast<InnerNode*>(node);
            uint64_t   nextID = inner->getChild(key);

            // lock coupling: fix child before releasing parent
            BufferFrame& bfNew = bm.fixPage(nextID, exclusive);
            bm.unfixPage(bf, false);
            bf = bfNew;

            node = static_cast<Node*>(bf.getData());
        }

        assert(node->isLeaf());
        *leafPtr = reinterpret_cast<LeafNode*>(node);
        return bf;
    }

    uint64_t reservePage() {
        // size is atomic
        // TODO: | segmentID + shiften
        return size++;
    }
};

#endif  // BTREE_H_


#ifndef HASHJOIN_H_
#define HASHJOIN_H_

#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../Operator.hpp"

using namespace std;

class HashJoin: public Operator {
    Operator&                                 op1;
    Operator&                                 op2;
    unsigned                                  id1;
    unsigned                                  id2;
    vector<Register*>                         regs;
    unordered_map<Register,vector<Register*>> map;

  public:
    HashJoin(Operator& op1, Operator& op2, unsigned id1, unsigned id2);
    void              open();
    bool              next();
    vector<Register*> getOutput();
    void              close();
};

HashJoin::HashJoin(Operator& op1, Operator& op2, unsigned id1, unsigned id2) :
    op1(op1), op2(op2), id1(id1), id2(id2) {}

void HashJoin::open() {
    op1.open();
    op2.open();

    // Build hash map with op1
    // TODO: external memory case
    while (op1.next()) {
        vector<Register*> regs1 = op1.getOutput();
        map.insert({*regs1[id1],regs1});
    }
}

bool HashJoin::next() {
    // probe op2
    while (op2.next()) {
        // check hash
        vector<Register*> regs2 = op2.getOutput();
        auto got = map.find(*regs2[id2]);
        if (got == map.end()) {
            continue; // no match
        }
        vector<Register*> regs1 = got->second;
        regs.clear();
        regs.reserve(regs1.size() + regs2.size());
        // append regs1
        regs.insert (regs.end(),regs1.begin(),regs1.end());
        // append regs2
        regs.insert (regs.end(),regs2.begin(),regs2.end());

        return true;
    }
    return false;
}

vector<Register*> HashJoin::getOutput() {
    return regs;
}

void HashJoin::close() {
    op1.close();
    op2.close();
}

#endif  // HASHJOIN_H_

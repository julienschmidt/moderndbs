#ifndef SELECTION_H_
#define SELECTION_H_

#include <stdexcept>
#include <vector>

#include "../Operator.hpp"

class Selection: public Operator {
    Operator&              op;
    unsigned               regID;
    Register*              constant;
    std::vector<Register*> regs;

  public:
    Selection(Operator& op, unsigned regID, Register* constant);
    void                   open();
    bool                   next();
    std::vector<Register*> getOutput();
    void                   close();
};

Selection::Selection(Operator& op, unsigned regID, Register* constant) :
    op(op), regID(regID), constant(constant) {}

void Selection::open() {
    op.open();
}

bool Selection::next() {
    while (op.next()) {
        regs = op.getOutput();
        if (regID < regs.size()) {
            if (*regs[regID] == *constant) {
                return true;
            }
            continue;
        }
        throw std::out_of_range("index out of range");
    }
    return false;
}

std::vector<Register*> Selection::getOutput() {
    return regs;
}

void Selection::close() {
    op.close();
}

#endif  // SELECTION_H_

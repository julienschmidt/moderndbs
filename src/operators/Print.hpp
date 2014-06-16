#ifndef PRINT_H_
#define PRINT_H_

#include <ostream>
#include <vector>

#include "../Operator.hpp"

class Print: public Operator {
    Operator&     op;
    std::ostream& out;

  public:
    Print(Operator& op, std::ostream& out);
    void                   open();
    bool                   next();
    std::vector<Register*> getOutput();
    void                   close();
};

Print::Print(Operator& op, std::ostream& out) : op(op), out(out) {}

void Print::open() {
    op.open();
}

bool Print::next() {
    return op.next();
}

std::vector<Register*> Print::getOutput() {
    std::vector<Register*> regs = op.getOutput();

    // prints registers in a pipe-separated-values-ish format
    for (int i = 0; i < regs.size(); ++i) {
        out << *regs[i] << "|";
    }
    out << std::endl;

    return regs;
}

void Print::close() {
    op.close();
}

#endif  // PRINT_H_

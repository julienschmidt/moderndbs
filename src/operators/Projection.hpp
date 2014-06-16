#ifndef PROJECTION_H_
#define PROJECTION_H_

#include <stdexcept>
#include <vector>

#include "../Operator.hpp"

class Projection: public Operator {
    Operator&              op;
    std::vector<unsigned>  IDs;

  public:
    Projection(Operator& op, std::vector<unsigned> IDs);
    void                   open();
    bool                   next();
    std::vector<Register*> getOutput();
    void                   close();
};

Projection::Projection(Operator& op, std::vector<unsigned> IDs) : op(op), IDs(IDs) {}

void Projection::open() {
    op.open();
}

bool Projection::next() {
    return op.next();
}

std::vector<Register*> Projection::getOutput() {
    std::vector<Register*> regs = op.getOutput();

    std::vector<Register*> out;
    out.reserve(IDs.size());

    for (unsigned i = 0; i < IDs.size(); ++i) {
        unsigned id = IDs[i];
        if (id >= regs.size()) {
            throw std::out_of_range("index out of range");
        }
        out.push_back(regs[id]);
    }

    return out;
}

void Projection::close() {
    op.close();
}

#endif  // PROJECTION_H_

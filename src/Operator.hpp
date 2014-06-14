#ifndef OPERATOR_H_
#define OPERATOR_H_

#include <vector>
#include "Register.hpp"

// Pure virtual interface class for operators
class Operator {
    std::vector<Register*> output;

  public:
    virtual void open() = 0;
    virtual bool next() = 0;
    virtual std::vector<Register*> getOutput() = 0;
    virtual void close() = 0;
};

#endif  // OPERATOR_H_

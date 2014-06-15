#ifndef REGISTER_H_
#define REGISTER_H_

#include <stdexcept>
#include <string>

#include "Types.hpp"

using namespace std;

class Register {
    Types::Tag type;
    void*      value;

  public:
    bool    isInteger() const;
    int64_t getInteger() const;
    void    setInteger(const int64_t i);

    bool    isString() const;
    string  getString() const;
    void    setString(const string& s);

    bool    operator<(const Register&);
    bool    operator==(const Register&);
    size_t  computeHash();
};

bool Register::isInteger() const {
    return type == Types::Tag::Integer;
}

int64_t Register::getInteger() const {
    static_assert(
        sizeof(void*) == sizeof(int64_t),
        "int must be same size as pointers"
    );

    return reinterpret_cast<int64_t>(value);
}

void Register::setInteger(int64_t i) {
    type  = Types::Tag::Integer;
    value = reinterpret_cast<void*>(i);
}

bool Register::isString() const {
    return type == Types::Tag::String;
}

string Register::getString() const {
    if (type != Types::Tag::String || value == NULL) {
        throw runtime_error("Not a valid string");
    }
    return *reinterpret_cast<string*>(value);
}

void Register::setString(const string& s) {
    type  = Types::Tag::String;
    value = (void*) &s;
}

bool Register::operator<(const Register& rhs) {
    if (type != rhs.type) {
        throw invalid_argument("Can not compare values of different types");
    }

    switch (type) {
    case Types::Tag::Integer:
        return getInteger() < rhs.getInteger();

    case Types::Tag::String:
        return getString().compare(rhs.getString()) < 0;

    default:
        throw logic_error("Unknown type");
    }
}

bool Register::operator==(const Register& rhs) {
    if (type != rhs.type) {
        throw invalid_argument("Can not compare values of different types");
    }

    switch (type) {
    case Types::Tag::Integer:
        return getInteger() == rhs.getInteger();

    case Types::Tag::String:
        return getString().compare(rhs.getString()) == 0;

    default:
        throw logic_error("Unknown type");
    }
}

size_t Register::computeHash() {
    switch (type) {
    case Types::Tag::Integer:
        hash<int64_t> intHasher;
        return intHasher(getInteger());

    case Types::Tag::String:
        hash<string> strHasher;
        return strHasher(getString());

    default:
        throw logic_error("Unknown type");
    }
}

#endif  // REGISTER_H_

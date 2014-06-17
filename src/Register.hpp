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
    void    load(Types::Tag, void*);

    bool    isInteger() const;
    int64_t getInteger() const;
    void    setInteger(const int64_t i);

    bool    isString() const;
    string  getString() const;
    void    setString(const string& s);

    bool    operator<(const Register&) const;
    bool    operator==(const Register&) const;
    size_t  computeHash() const;

  friend ostream& operator<< (ostream& out, const Register& reg);
};

void Register::load(Types::Tag type, void* ptr) {
    switch (type) {
    case Types::Tag::Integer:
        setInteger(*reinterpret_cast<int64_t*>(ptr));
        return;

    case Types::Tag::Char: {
        setString(reinterpret_cast<char*>(ptr));
        return;
    }

    default:
        throw logic_error("Unknown type");
    }
}

bool Register::isInteger() const {
    return type == Types::Tag::Integer;
}

int64_t Register::getInteger() const {
    // We can store the value directly instead of using a pointer when pointers
    // and the integer type have the same size
    static_assert(
        sizeof(void*) == sizeof(int64_t),
        "int64_t must be same size as pointers"
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
    return *static_cast<string*>(value);
}

void Register::setString(const string& s) {
    type  = Types::Tag::String;
    value = new string(s);
}

bool Register::operator<(const Register& rhs) const {
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

bool Register::operator==(const Register& rhs) const {
    if (type != rhs.type) {
        //throw invalid_argument("Can not compare values of different types");
        return false;
    }

    switch (type) {
    case Types::Tag::Integer:
        return getInteger() == rhs.getInteger();

    case Types::Tag::String:
        return getString().compare(rhs.getString()) == 0;

    default:
        //throw logic_error("Unknown type");
        return false;
    }
}

size_t Register::computeHash() const {
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

ostream& operator<<(ostream& out, const Register& reg) {
    switch (reg.type) {
    case Types::Tag::Integer:
        out << reg.getInteger();
        return out;

    case Types::Tag::String:
        out << reg.getString();
        return out;

    default:
        throw logic_error("Unknown type");
    }
}

// implement hash func for std::hash (adaptor for computeHash func)
namespace std {
    template<>
    struct hash<Register> {
        size_t operator()(Register const& reg) const {
            return reg.computeHash();
        }
    };
}

#endif  // REGISTER_H_

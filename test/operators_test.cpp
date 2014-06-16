
#include <cassert>
#include <iostream>
#include <string>

#include "../src/operators/TableScan.hpp"
#include "../src/BufferManager.hpp"
#include "../src/Register.hpp"
#include "../src/Schema.hpp"
#include "../src/SPSegment.hpp"

const vector<string> names = {
    "Anna",
    "Bob",
    "Chris",
    "Jeremy-Pascal",
    "Estefania-Celenta",
    "Rambo Ramon Rainer"
};

void testRegister() {
    Register reg{};

    reg.setInteger(42);
    assert(reg.isInteger());
    assert(reg.getInteger() == 42);

    assert(!reg.isString());
    reg.setString("foobar");
    assert(reg.isString());
    assert(reg.getString().compare("foobar") == 0);
}

int main(int argc, char* argv[]) {
    testRegister();

    BufferManager    bm(100);
    SPSegment        sp(bm, 1);
    Schema::Relation rel("Relation");

    // Create a Relation
    Schema::Relation::Attribute attr1{};
    attr1.name = "Int1";
    attr1.type = Types::Tag::Integer;
    attr1.len  = sizeof(int64_t);
    rel.attributes.push_back(attr1);

    Schema::Relation::Attribute attr2{};
    attr2.name = "Int2";
    attr2.type = Types::Tag::Integer;
    attr2.len  = sizeof(int64_t);
    rel.attributes.push_back(attr2);

    Schema::Relation::Attribute attr3{};
    attr3.name = "Name";
    attr3.type = Types::Tag::Char;
    attr3.len  = 32;
    rel.attributes.push_back(attr3);

    // Fill Relation with some values
    const size_t  recordSize  = 2*sizeof(int64_t)+32;
    const int64_t recordCount = 10;
    for (int64_t i = 0; i < recordCount; ++i) {
        char data[recordSize];
        int64_t* intPtr = reinterpret_cast<int64_t*>(data);
        *intPtr = i;
        intPtr++;
        *intPtr = recordCount-1-i;
        intPtr++;
        char* namePtr = reinterpret_cast<char*>(intPtr);
        memset(namePtr, '\0', 32);
        strcpy(namePtr, names[i%names.size()].c_str());
        sp.insert(Record(recordSize, data));
    }

    TableScan ts(rel, sp);
    ts.open();
    int64_t j = 0;
    while (ts.next()) {
        vector<Register*> regs = ts.getOutput();
        assert(regs.size() == 3);
        assert(regs[0]->isInteger());
        assert(regs[0]->getInteger() == j);
        assert(regs[1]->isInteger());
        assert(regs[1]->getInteger() == recordCount-1-j);
        assert(regs[2]->isString());
        assert(regs[2]->getString().compare(names[j%names.size()]) == 0);
        j++;
    }
    assert(j == recordCount);
    ts.close();

    std::cout << "TEST SUCCESSFUL!" << std::endl;
    return EXIT_SUCCESS;
}


#include <cassert>
#include <iostream>
#include <string>

#include "../src/Register.hpp"

int main(int argc, char* argv[]) {
   Register reg{};

   reg.setInteger(42);
   assert(reg.isInteger());
   assert(reg.getInteger() == 42);

   assert(!reg.isString());
   reg.setString("foobar");
   assert(reg.isString());
   assert(reg.getString().compare("foobar") == 0);

   std::cout << "TEST SUCCESSFUL!" << std::endl;
   return EXIT_SUCCESS;
}

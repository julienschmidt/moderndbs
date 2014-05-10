#include <iostream>
#include <memory>
#include <string>

#include "../src/Parser.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <sqlFile>" << endl;
        exit(EXIT_FAILURE);
    }

    // then try to open the files with the given file paths
    /*int fdInput;
    if((fdInput = open(argv[1], O_RDONLY)) < 0) {
        perror("Can not open input file");
        exit(EXIT_FAILURE);
    }*/


    Parser parser(argv[1]);
    unique_ptr<Schema> schema = parser.parse();

    cout << schema->toString() << endl;

    return EXIT_SUCCESS;
}

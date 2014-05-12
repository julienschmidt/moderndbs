#include <iostream>
#include <memory>
#include <string>

#include "../src/BufferManager.hpp"
#include "../src/Parser.hpp"
#include "../src/SchemaSegment.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <sqlFile>" << endl;
        exit(EXIT_FAILURE);
    }

    try {
        Parser parser(argv[1]);
        Schema schema = parser.parse();

        BufferManager bm(1);

        // store
        SchemaSegment ss0(bm, 0, schema);

        // load
        SchemaSegment ss1(bm, 0);

        // check wether inserted == loaded
        if(schema.toString().compare(ss1.getSchema()->toString()) == 0) {
            cout << "TEST SUCCESSFUL!" << endl;
            return EXIT_SUCCESS;
        }
    } catch(ParserError& pe) {
        cerr << "ParserError: " << pe.what() << endl;
    } catch(exception& e) {
        cerr << e.what() << endl;
    }

    cerr << "TEST FAILED!" << endl;
    return EXIT_FAILURE;
}

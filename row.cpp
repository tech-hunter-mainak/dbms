// Row.h
#include <string>
#include <vector>

using namespace std;

class Row {
public:
    string id;
    vector<string> values;

    // Default constructor
    Row() : id(""), values() {}

    // Parameterized constructor
    Row(const string &id, const vector<string> &values) : id(id), values(values) {}

    ~Row() {}
};


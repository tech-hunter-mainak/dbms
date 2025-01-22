#include "./validity.cpp"

int main() {
    string query;
    cout << "Enter a query: ";
    getline(cin, query); // Use getline to read the entire line

    if (validate(query)) {
        cout << "Valid query" << endl;
    } else {
        cout << "Invalid query" << endl;
    }

    return 0;
}

#include "./split.cpp"
using namespace std;

bool validate(string query) {
    vector<string> words = split(query);

    cout << "Words: " << endl; // Optional debugging statement
    for (int i = 0; i < words.size(); i++) {
        cout << words[i] << endl;
    }

    if (words.empty()) return false; // Query cannot be empty

    for (int i = 0; i < words.size(); i++) {
        if (words[i] == "and" || words[i] == "or" || words[i] == "not") {
            // Check if the operator is at the start or end
            if (i == 0 || i == words.size() - 1) {
                return false;
            }
            // Check for consecutive operators
            if (words[i - 1] == "and" || words[i - 1] == "or" || words[i - 1] == "not" ||
                words[i + 1] == "and" || words[i + 1] == "or" || words[i + 1] == "not") {
                return false;
            }
        }
    }
    return true;
}

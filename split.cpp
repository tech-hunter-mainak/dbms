#include <iostream>
#include <string>
#include <vector>
using namespace std;

vector<string> split(string query) {
    vector<string> words;
    string word = "";
    for (int i = 0; i < query.length(); i++) {
        if (query[i] == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word = "";
            }
        } else {
            //convert to lowercase using tolower
            if (query[i] >= 'A' && query[i] <= 'Z') {
                query[i] = tolower(query[i]);
            }
            word += query[i];
        }
    }
    if (!word.empty()) { // Add the last word
        words.push_back(word);
    }
    return words;
}

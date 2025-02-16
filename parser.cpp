#include "library.cpp"
#include "keywords.cpp"

class parser
{
private:
    string query = "";
    vector<string> queryList;
public:
    parser(string str);
    vector<string> split() {
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
    ~parser();
};

parser::parser(string str = "")
{
    if(str != "") {
        query = str;
    }
    queryList = split();
    for(auto s : queryList) {
        if(s == CREATE) {}
    }
}

parser::~parser()
{
    query = "";
}

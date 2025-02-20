#include "library.cpp"
#include "keywords.cpp"

class parser
{
private:
    string query = "";
    vector<string> queryList;

public:
    parser(string str);
    vector<string> split()
    {
        vector<string> words;
        string word = "";
        for (int i = 0; i < query.length(); i++)
        {
            if (query[i] == ' ')
            {
                if (!word.empty())
                {
                    words.push_back(word);
                    word = "";
                }
            }
            else
            {
                // convert to lowercase using tolower
                if (query[i] >= 'a' && query[i] <= 'z')
                {
                    query[i] = toupper(query[i]);
                }
                word += query[i];
            }
        }
        if (!word.empty())
        { // Add the last word
            words.push_back(word);
        }
        return words;
    }
    ~parser();
};

parser::parser(string str = "")
{
    if (str != "")
    {
        query = str;
    }
    queryList = split();
    for (auto s : queryList)
    {
        if (s == INIT)
        {
        }
    }
}

parser::~parser()
{
    query = "";
}

class table
{
private:
public:
    table();
    ~table();
};
table::table() {}
table::~table() {}

class database
{
private:
public:
    database();
    ~database();
};
database::database() {}
database::~database() {}

/// @brief ///////////////////////
//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
const string FILENAME = "data.csv";

// Function to check if a file exists
bool fileExists() {
    ifstream file(FILENAME);
    return file.good();
}

// Function to create a CSV file with a header if it doesn't exist
void createCSVIfNotExists() {
    if (!fileExists()) {
        ofstream file(FILENAME);
        if (file.is_open()) {
            file << "ID,Name,Age,Department\n"; // Adding header
            file.close();
        }
    }
}

// Function to extract values from 'insert("value1", "value2", ...)'
vector<string> extractValues(const string &command) {
    vector<string> values;
    
    size_t start = command.find('(');
    size_t end = command.find(')');
    
    if (start == string::npos || end == string::npos || start > end) {
        cout << "Invalid command format. Use: insert(\"value1\", \"value2\", ...)\n";
        return values;
    }

    string data = command.substr(start + 1, end - start - 1); // Extract inside ()
    
    stringstream ss(data);
    string item;
    
    while (getline(ss, item, ',')) {
        // Trim spaces and remove quotes
        item.erase(remove(item.begin(), item.end(), '"'), item.end());
        item.erase(0, item.find_first_not_of(' '));
        item.erase(item.find_last_not_of(' ') + 1);
        
        values.push_back(item);
    }
    
    return values;
}

// Function to insert a row
void insertRow(const string &command) {
    vector<string> values = extractValues(command);
    
    if (values.empty()) return;

    ofstream file(FILENAME, ios::app);
    if (file.is_open()) {
        for (size_t i = 0; i < values.size(); i++) {
            file << values[i];
            if (i < values.size() - 1) file << ",";
        }
        file << "\n";
        file.close();
        cout << "Data inserted successfully.\n";
    } else {
        cout << "Error opening file.\n";
    }
}

// Function to display file contents
void displayCSV() {
    ifstream file(FILENAME);
    if (!file.is_open()) {
        cout << "Error opening file.\n";
        return;
    }

    string line;
    while (getline(file, line)) {
        cout << line << endl;
    }
    file.close();
}

// Function to delete a row based on ID
void deleteRow(const string &id) {
    ifstream file(FILENAME);
    ofstream temp("temp.csv");
    bool found = false;
    
    if (!file.is_open() || !temp.is_open()) {
        cout << "Error opening file.\n";
        return;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string rowId;
        getline(ss, rowId, ',');

        if (rowId != id) {
            temp << line << "\n";
        } else {
            found = true;
        }
    }

    file.close();
    temp.close();
    remove(FILENAME.c_str());
    rename("temp.csv", FILENAME.c_str());

    if (found)
        cout << "Row deleted successfully.\n";
    else
        cout << "ID not found.\n";
}

// Function to delete all rows except the header
void deleteAllExceptHeader() {
    ifstream file(FILENAME);
    ofstream temp("temp.csv");

    if (!file.is_open() || !temp.is_open()) {
        cout << "Error opening file.\n";
        return;
    }

    string header;
    getline(file, header);
    temp << header << "\n"; // Keep header

    file.close();
    temp.close();
    remove(FILENAME.c_str());
    rename("temp.csv", FILENAME.c_str());

    cout << "All rows deleted except the header.\n";
}

// Function to delete the entire file
void deleteFile() {
    if (remove(FILENAME.c_str()) == 0) {
        cout << "File deleted successfully.\n";
    } else {
        cout << "Error deleting file.\n";
    }
}

// Function to update a value in the CSV
void updateValue(const string &id, int column, const string &newValue) {
    ifstream file(FILENAME);
    ofstream temp("temp.csv");
    bool found = false;

    if (!file.is_open() || !temp.is_open()) {
        cout << "Error opening file.\n";
        return;
    }

    string line;
    bool isHeader = true;
    while (getline(file, line)) {
        stringstream ss(line);
        vector<string> values;
        string item;
        while (getline(ss, item, ',')) {
            values.push_back(item);
        }

        if (isHeader || values[0] != id) {
            temp << line << "\n";
        } else {
            found = true;
            values[column] = newValue;
            for (size_t i = 0; i < values.size(); i++) {
                temp << values[i];
                if (i < values.size() - 1) temp << ",";
            }
            temp << "\n";
        }
        isHeader = false;
    }

    file.close();
    temp.close();
    remove(FILENAME.c_str());
    rename("temp.csv", FILENAME.c_str());

    if (found)
        cout << "Value updated successfully.\n";
    else
        cout << "ID not found.\n";
}

// Function to display menu
// void menu() {
//     createCSVIfNotExists();

//     while (true) {
//         cout << "\nMenu:\n";
//         cout << "1. Insert data (insert(\"val1\", \"val2\", ...))\n";
//         cout << "2. Display data\n";
//         cout << "3. Delete row by ID\n";
//         cout << "4. Delete all except header\n";
//         cout << "5. Delete entire file\n";
//         cout << "6. Update value by ID\n";
//         cout << "7. Exit\n";
//         cout << "Enter choice: ";

//         int choice;
//         cin >> choice;
//         cin.ignore();

//         if (choice == 1) {
//             cout << "Enter command (insert(\"value1\", \"value2\", ...)):\n> ";
//             string command;
//             getline(cin, command);
//             insertRow(command);
//         } 
//         else if (choice == 2) {
//             displayCSV();
//         } 
//         else if (choice == 3) {
//             cout << "Enter ID to delete: ";
//             string id;
//             cin >> id;
//             deleteRow(id);
//         } 
//         else if (choice == 4) {
//             deleteAllExceptHeader();
//         } 
//         else if (choice == 5) {
//             deleteFile();
//         } 
//         else if (choice == 6) {
//             cout << "Enter ID to update: ";
//             string id;
//             cout << "\nEnter column index (0-based): ";
//             int column;
//             cin >> column;
//             cout << "Enter new value: ";
//             string newValue;
//             cin >> newValue;
//             updateValue(id, column, newValue);
//         } 
//         else if (choice == 7) {
//             cout << "Exiting program...\n";
//             break;
//         } 
//         else {
//             cout << "Invalid choice. Try again.\n";
//         }
//     }
// }


queue<string> input() {
    string inputStr, word = "";
    getline(cin, inputStr);
    
    queue<string> query;
    stack<char> symbol;
    bool s_quotation = false, d_quotation = false, inside_parentheses = false;
    string parentheses_content = "";

    for (char ch : inputStr) {
        switch (ch) {
            case '!':
            case '<':
            case '>':
            case '=':
            case ',':
                if (!s_quotation && !d_quotation) {
                    if (!word.empty()) {
                        query.push(word);
                        word.clear();
                    }
                    query.push(string(1, ch));  // Store operators separately
                } else {
                    word.push_back(ch);
                }
                break;
            case ' ':
                if (!s_quotation && !d_quotation) {  // If not inside quotes, push the word
                    if (!word.empty()) query.push(word), word.clear();
                } else word += ch;  // Otherwise, treat it as part of the word
                break;

            case '(':
                if (!s_quotation && !d_quotation) {
                    symbol.push(ch);
                    inside_parentheses = true;
                    parentheses_content = "";
                } else word += ch;
                break;

            case ')':
                if (!s_quotation && !d_quotation) {
                    if (symbol.empty() || symbol.top() != '(') 
                        throw logic_error("Mismatched parentheses");
                    symbol.pop();
                    inside_parentheses = false;

                    // Check if parentheses contain only numbers or are enclosed in quotes
                    bool valid = true;
                    if (!parentheses_content.empty() && !s_quotation && !d_quotation) {
                        for (char pc : parentheses_content) {
                            if (!isdigit(pc)) {
                                valid = false;
                                break;
                            }
                        }
                    }
                    if (!valid) throw invalid_argument("Syntax error: Invalid content inside parentheses");
                } else word += ch;
                break;

            case '\'':
                if (!d_quotation) s_quotation = !s_quotation;
                else word += ch;
                break;

            case '\"':
                if (!s_quotation) d_quotation = !d_quotation;
                else word += ch;
                break;

            default:
                if (inside_parentheses && !s_quotation && !d_quotation) {
                    parentheses_content += ch;
                }
                if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' || ch <= '9')) {
                    word += ch;
                }
                else {
                    throw invalid_argument(ch + " is not expected here");
                }
        }
    }
    
    if (!word.empty()) query.push(word);
    if (!symbol.empty()) throw invalid_argument("Invalid expressions");

    // Print queue content for verification
    // while (!query.empty()) {
    //     cout << query.front() << endl;
    //     query.pop();
    // }
    return query;
}

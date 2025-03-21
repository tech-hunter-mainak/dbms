#include "library.cpp"
#include "keywords.cpp"
#include "initiateData.cpp"

extern string fs_path;         // Root DBMS folder path
extern string currentDatabase; // Currently selected database name (empty if none)
extern string currentTable;    // Currently selected table name (empty if none)

class parser {
public:
    parser(list<string> &queryList);
    ~parser();
};

parser::parser(list<string> &queryList) {
    while (!queryList.empty()) {
        string query = queryList.front();
        queryList.pop_front();
    
        if (query == INIT) {
            // INIT <database_name>
            if(queryList.empty()){
                cout << "INIT: Database name expected." << endl;
                continue;
            }
            string dbName = queryList.front();
            queryList.pop_front();
            init_database(dbName);
        }
        else if (query == MAKE) {
            if (queryList.empty()) {
                cout << "MAKE: Table name expected." << endl;
                continue;
            }
            make_table(queryList);
        }                        
        else if (query == ERASE) {
            // Expect a name (database or table) to erase.
            if(queryList.empty()){
                if(currentDatabase.empty())
                    cout << "ERASE: Database name expected." << endl;
                else
                    cout << "ERASE: Table name expected." << endl;
                continue;
            }
            string name = queryList.front();
            queryList.pop_front();
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected token after name in ERASE command" << endl;
                return;
            }
            
            // At root level (no database selected): erase a database.
            if(currentDatabase.empty()){
                if(eraseDatabase(name)){
                    // Optionally, if the erased database was current (shouldn't happen if you're at root)
                    if(currentDatabase == name)
                        currentDatabase = "";
                }
            }
            // Inside a database: erase a table.
            else {
                if(eraseTable(name)){
                    if(currentTable == name)
                        currentTable = "";
                }
            }
        }
        
        else if (query == CLEAN) {
            if(currentTable == ""){
                cout<<"please choose a table before cleaning"<<endl;
            } else{
                    cleanTable();
            }
        }
        else if (query == DEL) {
            // DEL <ID>
            if (queryList.empty()) {
                cout << "DEL: No ID provided." << endl;
                continue;
            }
            string id = queryList.front();
            queryList.pop_front();
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected token after ID in DEL command" << endl;
                return;
            }
            deleteRow(id);
        }
        else if (query == CHANGE) {
            // CHANGE <ID> <column_name> <new_value>
            if (queryList.size() < 3) {
                cerr << "CHANGE: Not enough tokens provided." << endl;
                return;
            }
            string id = queryList.front();
            queryList.pop_front();
            string colName = queryList.front();
            queryList.pop_front();
            string newValue = queryList.front();
            queryList.pop_front();
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected tokens after CHANGE command" << endl;
                return;
            }
            // Determine the column index by searching the headers.
            int colIndex = -1;
            for (int i = 0; i < headers.size(); i++) {
                if (headers[i] == colName) {
                    colIndex = i;
                    break;
                }
            }
            if(colIndex == -1){
                cout << "Column \"" << colName << "\" not found." << endl;
            } else {
                updateValue(id, colIndex, newValue);
            }
        }
        else if (query == INSERT) {
            // Expect one or more VALUES clauses.
            while (!queryList.empty()) {
                string token = queryList.front();
                queryList.pop_front();
                if (token != "VALUES") {
                    cout << "Unexpected token in INSERT command: " << token << endl;
                    continue;
                }
                // The next token should be the parenthesized content (already grouped by input()).
                if (queryList.empty()){
                    cout << "VALUES: expected parenthesized list after VALUES keyword." << endl;
                    break;
                }
                string valuesToken = queryList.front();
                queryList.pop_front();
                // Reconstruct a command string that insertRow() understands.
                // We add back the parentheses for extractValues() to work.
                string insertCommand = "INSERT(" + valuesToken + ")";
                insertRow(insertCommand);
            }
        }
        
                
        else if (query == ENTER) {
            // ENTER <database_name>
            if(queryList.empty()){
                cout << "ENTER: Database name expected." << endl;
                continue;
            }
            string db_name = queryList.front();
            queryList.pop_front();
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected token after database name in ENTER command" << endl;
                return;
            }
            if(fs::current_path().string() == fs_path) {
                use(db_name, "dir");
                currentDatabase = db_name;  // Updated: Set currentDatabase
                currentTable = "";          // Reset currentTable
            }
        }
        else if (query == CHOOSE) {
            // CHOOSE <table_name>
            if(queryList.empty()){
                cout << "CHOOSE: Table name expected." << endl;
                continue;
            }
            string table_name = queryList.front();
            queryList.pop_front();
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected token after table name in CHOOSE command" << endl;
                return;
            }
            
            // Only allow CHOOSE if we are inside a database directory.
            if(fs::current_path().string() != fs_path){
                string filename = table_name + ".csv";
                ifstream file(filename);
                if(!file.good()){
                    cout << "Table " << table_name << " does not exist." << endl;
                } else {
                    use(table_name, "table");
                    currentTable = table_name; 
                }
            }
        }        
        else if (query == OR) {
            cout << "OR operator not implemented separately." << endl;
        }
        else if (query == AND) {
            cout << "AND operator not implemented separately." << endl;
        }
        else if (query == WHERE) {
            cout << "WHERE clause not implemented separately." << endl;
        }
        else if (query == LIKE) {
            cout << "LIKE operator not implemented." << endl;
        }
        else if (query == EXIT) {
            // EXIT behaves like BACK here.
            move_up();
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected token after EXIT command" << endl;
                return;
            }
        }
        else if (query == SHOW) {
            // SHOW <parameters...>
            string params;
            while (!queryList.empty()) {
                params += queryList.front() + " ";
                queryList.pop_front();
            }
            if (!params.empty())
                params.pop_back();
            show(params);
        }
        else if(query == ROLLBACK) {
            if(currentTable.empty()){
                cout << "No table selected for transaction rollback." << endl;
                return;
            }
            rollbackTransaction(currentTable);
        }
        else if(query == COMMIT) {
            if(currentTable.empty()){
                cout << "No table selected for transaction commit." << endl;
                return;
            }
            commitTransaction(currentTable);
        }
        else {
            cout << "Unknown command: " << query << endl;
        }
    }
}

parser::~parser() { }


class table
{
public:
    table();
    ~table();
};
table::table() {}
table::~table() {}

class database
{
public:
    database();
    ~database();
};
database::database() {}
database::~database() {}


// Function to check if a file exists
bool fileExists()
{
    ifstream file(FILENAME);
    return file.good();
}

// Function to create a CSV file with a header if it doesn't exist
void createCSVIfNotExists()
{
    if (!fileExists())
    {
        ofstream file(FILENAME);
        if (file.is_open())
        {
            file << "ID,Name,Age,Department\n"; // Adding header
            file.close();
        }
    }
}

// Function to display file contents
void displayCSV()
{
    ifstream file(FILENAME);
    if (!file.is_open())
    {
        cout << "Error opening file.\n";
        return;
    }

    string line;
    while (getline(file, line))
    {
        cout << line << endl;
    }
    file.close();
}




// Function to delete the entire file
void deleteFile()
{
    if (remove(FILENAME.c_str()) == 0)
    {
        cout << "File deleted successfully.\n";
    }
    else
    {
        cout << "Error deleting file.\n";
    }
}

list<string> input() {
    string inputStr;
    getline(cin, inputStr);

    list<string> query;
    string word = "";
    bool s_quotation = false, d_quotation = false;
    bool inside_parentheses = false;
    string parentheses_content = "";
    
    // Process each character in the input line.
    for (char ch : inputStr) {
        if (inside_parentheses) {
            // When inside parentheses, append everything until we see a closing parenthesis.
            if (!s_quotation && !d_quotation && ch == ')') {
                // End of parentheses; finish token.
                inside_parentheses = false;
                query.push_back(parentheses_content);
                parentheses_content = "";
            } else {
                // Append character regardless (spaces, commas, etc.)
                parentheses_content.push_back(ch);
            }
        } else {
            // Not inside parentheses.
            switch(ch) {
                case '(':
                    // Start a new parentheses token.
                    inside_parentheses = true;
                    // If there's a pending token in 'word', push it first.
                    if (!word.empty()) {
                        query.push_back(word);
                        word.clear();
                    }
                    // Start accumulating inner content.
                    parentheses_content = "";
                    break;
                case ' ':
                    // Use space as delimiter only when not inside parentheses or quotes.
                    if (!s_quotation && !d_quotation) {
                        if (!word.empty()) {
                            query.push_back(word);
                            word.clear();
                        }
                    } else {
                        word.push_back(ch);
                    }
                    break;
                case ',':
                    // Outside parentheses, discard comma as a separate token.
                    if (!s_quotation && !d_quotation) {
                        if (!word.empty()) {
                            query.push_back(word);
                            word.clear();
                        }
                    } else {
                        word.push_back(ch);
                    }
                    break;
                case '!':
                case '<':
                case '>':
                    // Punctuation tokens outside quotes.
                    if (!s_quotation && !d_quotation) {
                        if (!word.empty()) {
                            query.push_back(word);
                            word.clear();
                        }
                        query.push_back(string(1, ch));
                    } else {
                        word.push_back(ch);
                    }
                    break;
                case '\'':
                    if (!d_quotation) {
                        s_quotation = !s_quotation;
                    }
                    word.push_back(ch);
                    break;
                case '\"':
                    if (!s_quotation) {
                        d_quotation = !d_quotation;
                    }
                    word.push_back(ch);
                    break;
                default:
                    if (isalnum(ch)) {
                        // Outside quotes, convert alphabetic characters to uppercase.
                        if (!s_quotation && !d_quotation && isalpha(ch))
                            word.push_back(toupper(ch));
                        else
                            word.push_back(ch);
                    } else {
                        // For other punctuation outside parentheses, simply append.
                        word.push_back(ch);
                    }
                    break;
            }
        }
    }
    
    // If there's any pending token outside parentheses, push it.
    if (!word.empty()) {
        query.push_back(word);
        word.clear();
    }
    
    // If we ended while still inside parentheses, it's a mismatched error.
    if (inside_parentheses) {
        throw invalid_argument("Mismatched parentheses in input.");
    }
    
    // For debugging: print tokens
    for (const string &token : query) {
        cout << token << endl;
    }
    
    return query;
}

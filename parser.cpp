#include "library.cpp"
#include "keywords.cpp"
#include "initiateData.cpp"  // Contains the free function make_table (among others)
#include "table.cpp"         // Contains the Table class with encapsulated operations

extern string fs_path;         // Root DBMS folder path
extern string currentDatabase; // Currently selected database name (empty if none)
extern string currentTable;    // Currently selected table name (empty if none)
extern bool exitProgram;  // Declaration of the global variable

// Global pointer to the current Table instance.
Table* currentTableInstance = nullptr;


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
            if (queryList.empty()) {
                cout << "INIT: Database name expected." << endl;
                continue;
            }
            string dbName = queryList.front();
            queryList.pop_front();
            init_database(dbName);
        }
        else if (query == MAKE) {
            // MAKE <table_name> [ ( <column_definitions> ) ]
            if (queryList.empty()) {
                cout << "MAKE: Table name expected." << endl;
                continue;
            }
            // Call the free function make_table (defined in initiateData.cpp)
            make_table(queryList);
            // After creation, currentTable should be set by make_table.
            // Create a new Table instance to manage further operations.
            if (!currentTable.empty()) {
                if (currentTableInstance) {
                    delete currentTableInstance;
                    currentTableInstance = nullptr;
                }
                currentTableInstance = new Table(currentTable);
            }
        }
        else if (query == ERASE) {
            // ERASE <database or table name>
            if (queryList.empty()) {
                if (currentDatabase.empty())
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
            if (currentDatabase.empty()) {
                if (eraseDatabase(name)) {
                    if (currentDatabase == name)
                        currentDatabase = "";
                }
            }
            // Inside a database: erase a table.
            else {
                if (eraseTable(name)) {
                    if (currentTable == name)
                        currentTable = "";
                    if (currentTableInstance) {
                        delete currentTableInstance;
                        currentTableInstance = nullptr;
                    }
                }
            }
        }
        else if (query == CLEAN) {
            if (currentTable == "") {
                cout << "Please choose a table before cleaning." << endl;
            } else {
                if (currentTableInstance)
                    currentTableInstance->cleanTable();
                else
                    cout << "No table instance available for cleaning." << endl;
            }
        }
        else if (query == DEL) {
            if (queryList.empty()) {
                cout << "DEL: No arguments provided." << endl;
                continue;
            }
            // Check if deletion is based on conditions.
            if (queryList.front() == WHERE) {
                queryList.pop_front();  // Remove the WHERE token.
                // Copy the remaining tokens into a vector.
                vector<string> condTokens(queryList.begin(), queryList.end());
                // Clear the query list so tokens aren’t processed again.
                queryList.clear();
                // Call the advanced condition parser.
                vector<vector<Condition>> conditionGroups = parseAdvancedConditions(condTokens);
                if (currentTableInstance)
                    currentTableInstance->deleteRowsByAdvancedConditions(conditionGroups);
                else
                    cout << "No table selected for deletion." << endl;
            }
            else {
                // Otherwise, treat tokens as either row IDs or column names.
                vector<string> items;
                while (!queryList.empty()) {
                    string token = queryList.front();
                    queryList.pop_front();
                    // If the token contains commas, split it.
                    size_t pos = 0;
                    while ((pos = token.find(',')) != string::npos) {
                        string part = token.substr(0, pos);
                        if (!part.empty())
                            items.push_back(part);
                        token = token.substr(pos + 1);
                    }
                    if (!token.empty())
                        items.push_back(token);
                }
                // Process each item: trim whitespace and remove surrounding quotes.
                for (auto &item : items) {
                    // Trim whitespace.
                    item.erase(0, item.find_first_not_of(" \t"));
                    item.erase(item.find_last_not_of(" \t") + 1);
                    // Remove surrounding quotes if present.
                    if (!item.empty() && item.front() == '"' && item.back() == '"') {
                        item = item.substr(1, item.size() - 2);
                    }
                    if (currentTableInstance) {
                        if (currentTableInstance->hasRow(item))
                            currentTableInstance->deleteRow(item);
                        else if (currentTableInstance->hasColumn(item))
                            currentTableInstance->deleteColumn(item);
                        else
                            cout << "DEL: Neither a row with ID nor a column named \"" << item << "\" exists." << endl;
                    } else {
                        cout << "No table selected for deletion." << endl;
                    }
                }
            }
        }               
        else if (query == CHANGE) {
            // This branch supports two syntaxes:
            // Syntax 1: CHANGE <column_name> <old_value> TO <new_value> Where <condition>
            // Syntax 2: CHANGE <old_value> TO <new_value> Where <condition>
            if (queryList.empty()) {
                cerr << "CHANGE: Not enough tokens provided." << endl;
                return;
            }
            
            bool hasColumnSpecified = false;
            string colName, oldValue, newValue;
            
            // Peek at the first token.
            string firstToken = queryList.front();
            queryList.pop_front();
            
            // If the next token is "TO", then it's Syntax 2 (no column specified).
            if (!queryList.empty() && queryList.front() == "TO") {
                oldValue = firstToken;
                // Remove "TO"
                queryList.pop_front();
                if (queryList.empty()) {
                    cerr << "CHANGE: New value missing." << endl;
                    return;
                }
                newValue = queryList.front();
                queryList.pop_front();
            } else {
                // Otherwise, it's Syntax 1 (with column name).
                hasColumnSpecified = true;
                colName = firstToken;
                // Remove surrounding quotes if present.
                if (!colName.empty() && colName.front() == '"' && colName.back() == '"') {
                    colName = colName.substr(1, colName.size() - 2);
                }
                if (queryList.empty()) {
                    cerr << "CHANGE: Missing old value." << endl;
                    return;
                }
                oldValue = queryList.front();
                queryList.pop_front();
                if (queryList.empty() || queryList.front() != "TO") {
                    cerr << "CHANGE: Expected 'TO' after old value." << endl;
                    return;
                }
                // Remove "TO"
                queryList.pop_front();
                if (queryList.empty()) {
                    cerr << "CHANGE: Missing new value." << endl;
                    return;
                }
                newValue = queryList.front();
                queryList.pop_front();
            }
            
            // Optional: Look for a WHERE clause.
            vector<vector<Condition>> conditionGroups;
            if (!queryList.empty()) {
                string token = queryList.front();
                if (token == "where" || token == "WHERE") {
                    queryList.pop_front();  // Remove the WHERE token.
                    vector<string> condTokens(queryList.begin(), queryList.end());
                    conditionGroups = parseAdvancedConditions(condTokens);
                } else {
                    cerr << "Syntax error: unexpected token(s) after new value in CHANGE command" << endl;
                    return;
                }
            }
            
            if (!currentTableInstance) {
                cout << "No table selected for update." << endl;
                return;
            }
            
            // Call the appropriate Table method.
            if (hasColumnSpecified)
                currentTableInstance->updateValueByCondition(colName, oldValue, newValue, conditionGroups);
            else
                currentTableInstance->updateValueByCondition(oldValue, newValue, conditionGroups);
        }        
        else if (query == INSERT) {
            // Expect one or more VALUES clauses.
            while (!queryList.empty()) {
                string token = queryList.front();
                queryList.pop_front();
                if (token != VALUES) {
                    cout << "Unexpected token in INSERT command: " << token << endl;
                    continue;
                }
                if (queryList.empty()){
                    cout << "VALUES: expected parenthesized list after VALUES keyword." << endl;
                    break;
                }
                string valuesToken = queryList.front();
                queryList.pop_front();
                // Reconstruct a command string for insertRow.
                string insertCommand = "INSERT(" + valuesToken + ")";
                if (currentTableInstance)
                    currentTableInstance->insertRow(insertCommand);
                else
                    cout << "No table selected for insertion." << endl;
            }
        }
        else if (query == ENTER) {
            // ENTER <database_name>
            if (queryList.empty()) {
                cout << "ENTER: Database name expected." << endl;
                continue;
            }
            string db_name = queryList.front();
            queryList.pop_front();
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected token after database name in ENTER command" << endl;
                continue;
            }
            try {
                if (fs::exists(db_name) && fs::is_directory(db_name)) {
                    fs::current_path(db_name);
                    currentDatabase = db_name;
                    currentTable = "";
                    if (currentTableInstance) {
                        delete currentTableInstance;
                        currentTableInstance = nullptr;
                    }
                } else {
                    cout << "Database \"" << db_name << "\" does not exist. Create it using INIT command." << endl;
                }
            } catch (const fs::filesystem_error &e) {
                cerr << "Filesystem error in ENTER command: " << e.what() << endl;
            }
        }
        else if (query == CHOOSE) {
            // CHOOSE <table_name>
            if (queryList.empty()) {
                cout << "CHOOSE: Table name expected." << endl;
                continue;
            }
            string table_name = queryList.front();
            queryList.pop_front();
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected token after table name in CHOOSE command" << endl;
                continue;
            }
            if (currentDatabase.empty()) {
                cout << "No database selected. Use ENTER <database> before choosing a table." << endl;
                continue;
            }
            try {
                string filename = table_name + ".csv";
                ifstream file(filename);
                if (file.good()) {
                    file.close();
                    // Optionally, call use(table_name, "table") if needed.
                    currentTable = table_name;
                    if (currentTableInstance) {
                        delete currentTableInstance;
                        currentTableInstance = nullptr;
                    }
                    currentTableInstance = new Table(table_name);
                } else {
                    cout << "Table \"" << table_name << "\" does not exist." << endl;
                }
            } catch (const fs::filesystem_error &e) {
                cerr << "Filesystem error in CHOOSE command: " << e.what() << endl;
            }
        }
        else if (query == CRASH) {
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected token after CRASH command" << endl;
                return;
            }
            if (currentTableInstance) {
                delete currentTableInstance;
                currentTableInstance = nullptr;
            }
            currentTable = "";
            currentDatabase = "";
            cout << "CRASH: Forcing immediate termination of the DBMS session." << endl;
            exitProgram = true;
        }
        else if (query == EXIT) {
            if (!queryList.empty()) {
                cerr << "Syntax error: unexpected token after EXIT command" << endl;
                return;
            }
            move_up();
        }
        else if (query == LIST) {
            if (currentDatabase == "" && currentTable == "") {
                listDatabases();
            }
            else if (currentDatabase != "" && currentTable == "") {
                listTables();
            }
            else if (currentDatabase != "" && currentTable != "") {
                cout << "LIST command is not available in table context." << endl;
            }
        }
        else if (query == SHOW) {
            string params;
            while (!queryList.empty()) {
                params += queryList.front() + " ";
                queryList.pop_front();
            }
            if (!params.empty())
                params.pop_back();
            if (currentTableInstance)
                currentTableInstance->show(params);
            else
                cout << "No table selected for SHOW command." << endl;
        }
        else if (query == ROLLBACK) {
            if (currentTable.empty()){
                cout << "No table selected for transaction rollback." << endl;
                return;
            }
            if (currentTableInstance)
                currentTableInstance->rollbackTransaction();
            else
                cout << "No table instance available for rollback." << endl;
        }
        else if (query == COMMIT) {
            if (currentTable.empty()){
                cout << "No table selected for transaction commit." << endl;
                return;
            }
            if (currentTableInstance)
                currentTableInstance->commitTransaction();
            else
                cout << "No table instance available for commit." << endl;
        }
        else {
            cout << "Unknown command: " << query << endl;
        }
    }
}

parser::~parser() { }

list<string> input() {
    string inputStr;
    getline(cin, inputStr);

    list<string> query;
    string word = "";
    bool s_quotation = false, d_quotation = false;
    bool inside_parentheses = false;
    string parentheses_content = "";
    
    for (char ch : inputStr) {
        if (inside_parentheses) {
            // When inside parentheses, do NOT skip quotes; simply append them.
            if (!s_quotation && !d_quotation && ch == ')') {
                inside_parentheses = false;
                query.push_back(parentheses_content);
                parentheses_content = "";
            } else {
                parentheses_content.push_back(ch);
            }
        } else {
            switch(ch) {
                case '(':
                    inside_parentheses = true;
                    if (!word.empty()) {
                        query.push_back(word);
                        word.clear();
                    }
                    parentheses_content = "";
                    break;
                case ' ':
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
                    if (!s_quotation && !d_quotation) {
                        if (!word.empty()) {
                            query.push_back(word);
                            word.clear();
                        }
                    } else {
                        word.push_back(ch);
                    }
                    break;
                case '"':
                    // Toggle double quote flag and preserve the quote.
                    d_quotation = !d_quotation;
                    word.push_back(ch);
                    break;
                case '\'':
                    s_quotation = !s_quotation;
                    word.push_back(ch);
                    break;
                default:
                    if (!s_quotation && !d_quotation) {
                        if (isalpha(ch))
                            word.push_back(tolower(ch));
                        else
                            word.push_back(ch);
                    } else {
                        word.push_back(ch);
                    }
                    break;
            }
        }
    }
    
    if (!word.empty())
        query.push_back(word);
    if (inside_parentheses)
        throw invalid_argument("Mismatched parentheses in input.");
    
// For debugging: print tokens
    for (const string &token : query) {
        cout << token << endl;
    }
    
    return query;
}

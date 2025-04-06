#include "initiateData.cpp"  
#include "table.cpp"


extern string fs_path;         // Root DBMS folder path
extern string currentDatabase; // Currently selected database name (empty if none)
extern string currentTable;    // Currently selected table name (empty if none)
extern bool exitProgram;  // Declaration of the global variable

// Global pointer to the current Table instance.
Table* currentTableInstance = nullptr;
void exitTable() {
    if (!currentTableInstance)
        return;
    // If there are unsaved changes, ask the user whether to save.
    if (currentTableInstance->unsavedChanges) {
        cout << "You have unsaved changes. Do you want to save them? (y/n): ";
        string answer;
        getline(cin, answer);
        // Trim and convert to lowercase.
        transform(answer.begin(), answer.end(), answer.begin(), ::tolower);
        if (!answer.empty() && answer[0] == 'y') {
            currentTableInstance->commitTransaction();
        } else {
            cout << "Response: Discarding changes." << endl;
        }
    }
    delete currentTableInstance;
    currentTableInstance = nullptr;
    currentTable = "";      // Clear current table context.
    cout << "Response: Exited table" << endl;
}
class Parser {
private:
    list<string> queryList;
    // Helper functions for each command:
    void processInit() {
        // INIT <database_name>
        if (queryList.empty()) {
            cout << "Syntax Error: INIT -> Database name expected." << endl;
            return ;
        }
        string dbName = getCommand();
        checkExtraTokens();
        init_database(dbName);
    }
    void processMake() {
        // MAKE <table_name> [ ( <column_definitions> ) ]
        if(currentDatabase.empty()){
            cerr <<"Sys ERR: Enter a database to make a table." << endl;
            return ;
        }
        if(!currentTable.empty()){
            cerr <<"Sys ERR: MAKE cannot be used within a table." << endl;
            queryList.clear();
            return ;
        }
        if (queryList.empty()) {
            cout << "Syntax Error: MAKE -> No arguments provided." << endl;
            return;
        }
        string tableName = getCommand();    
        // Set the global currentTable to the table name.
        currentTable = tableName;
        // Call the free function make_table with only the header definitions.
        make_table(queryList);
        // After creation, currentTable should be set by make_table.
        // Create a new Table instance to manage further operations.
        if (!currentTable.empty()) {
            // if (currentTableInstance) {
            //     delete currentTableInstance;
            //     currentTableInstance = nullptr;
            // }
            // here stopeed 
            currentTableInstance = new Table(currentTable);
        }
    }    
    void processErase() {
        // ERASE <database or table name>
        if (queryList.empty()) {
            if (currentDatabase.empty())
                cout << "Syntax Error: ERASE -> Database name expected." << endl;
            else
                cout << "Syntax Error: ERASE -> Table name expected." << endl;
            return;
        }
        string name = getCommand();
        if(checkExtraTokens()) return;
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
                if (currentTableInstance) { //////////////    TABLE INSTANCE IS HERE
                    delete currentTableInstance;
                    currentTableInstance = nullptr;
                }
            }
        }
    }
    void processClean() {
        if (currentTable == "") {
            cout << "Sys Err: Please choose a table before cleaning." << endl;
        } else {
            if(checkExtraTokens()) return;
            if (currentTableInstance)
                currentTableInstance->cleanTable();
            else{   
                cout << "Program Error: This shouldn't happen contact dev." << endl;
                exitProgram = true;
            }  
        }
    }

    void processDel() {
        if(currentTable.empty() && !currentTableInstance){
            cerr << "Sys ERR: Please choose a table before deleting." << endl;
            queryList.clear();
            return ;
        }
        if (queryList.empty()) {
            cout << "Syntax Error: DEL -> No arguments provided." << endl;
            return;
        }
        // Check if deletion is based on conditions.
        if (queryList.front() == WHERE) {
            queryList.pop_front();  // Remove the WHERE token.
            // Copy the remaining tokens into a vector.
            vector<string> condTokens(queryList.begin(), queryList.end());
            // Clear the query list so tokens aren’t processed again.
            queryList.clear();
            // Call the advanced condition parser.
            vector<vector<Condition>> conditionGroups = currentTableInstance->parseAdvancedConditions(condTokens);
            if(conditionGroups.empty()) return;
            if (currentTableInstance)
                currentTableInstance->deleteRowsByAdvancedConditions(conditionGroups);
        }
        else {
            // Otherwise, treat tokens as either row IDs or column names.
            vector<string> items;
            while (!queryList.empty()) {
                string token = getCommand();
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
                if (!item.empty() && ((item.front() == '"' && item.back() == '"') || (item.front() == '\'' && item.back() == '\''))) {
                    item = item.substr(1, item.size() - 2);
                }
                if (currentTableInstance->hasRow(item)){
                    currentTableInstance->deleteRow(item);
                }
                else if (currentTableInstance->hasColumn(item))
                    currentTableInstance->deleteColumn(item);
                else
                    cout << "Logic Error: Neither a row with primaryKey nor a column named \"" << item << "\" exists." << endl;
            }
        }
    }
    void processDescribe(){
        if (currentTable.empty()) {
            cout << "Sys ERR: DESCRIBE -> can only be used in table" << endl;
            return ;
        }
        if(checkExtraTokens()) return;
        if (currentTableInstance)
            currentTableInstance->describe();
    }
    void processChange() {
        // This branch supports two syntaxes:
            // Syntax 1: CHANGE <column_name> <old_value> TO <new_value> Where <condition>
            // Syntax 2: CHANGE <old_value> TO <new_value> Where <condition>
            if (!currentTableInstance) {
                cout << "Sys ERR: No table selected for update." << endl;
                queryList.clear();
                return;
            }
            if (queryList.empty()) {
                cerr << "Syntax Error: CHANGE -> no arguments provided." << endl;
                return;
            }
            
            bool hasColumnSpecified = false;
            string colName, oldValue, newValue;
            string firstToken = getCommand();
            
            // If the next token is "TO", then it's Syntax 2 (no column specified).
            if (!queryList.empty() && queryList.front() == TO) {
                oldValue = firstToken;
                // Remove "TO"
                queryList.pop_front();
                if (queryList.empty()) {
                    cerr << "Syntax Error: CHANGE -> newValue is missing." << endl;
                    return;
                }
                newValue = getCommand();
            } else {
                // Otherwise, it's Syntax 1 (with column name).
                hasColumnSpecified = true;
                colName = firstToken;
                // Remove surrounding quotes if present.
                if (!colName.empty() && ((colName.front() == '"' && colName.back() == '"') || (colName.front() == '\'' && colName.back() == '\''))) {
                    colName = colName.substr(1, colName.size() - 2);
                }
                if (queryList.empty()) {
                    cerr << "Syntax Error: CHANGE -> Missing old value." << endl;
                    return;
                }
                oldValue = getCommand();
                if (queryList.empty() || queryList.front() != TO) {
                    cerr << "Syntax Error: CHANGE -> missing arguments" << endl;
                    queryList.clear();
                    return;
                }
                // Remove "TO"
                queryList.pop_front();
                if (queryList.empty()) {
                    cerr << "Syntax Error: CHANGE -> Missing new value." << endl;
                    return;
                }
                newValue = getCommand();
            }
            
            // Optional: Look for a WHERE clause.
            vector<vector<Condition>> conditionGroups;
            if (!queryList.empty()) {
                string token = queryList.front();
                if (token == WHERE) {
                    queryList.pop_front();  // Remove the WHERE token.
                    vector<string> condTokens(queryList.begin(), queryList.end());
                    queryList.clear();
                    conditionGroups = currentTableInstance->parseAdvancedConditions(condTokens);
                    if(conditionGroups.empty()) return;
                } else {
                    cerr << "Syntax error: unexpected token(s) after new value in CHANGE command" << endl;
                    return;
                }
            }
            // Call the appropriate Table method.
            if (hasColumnSpecified)
                currentTableInstance->updateValueByCondition(colName, oldValue, newValue, conditionGroups);
            else
                currentTableInstance->updateValueByCondition(oldValue, newValue, conditionGroups);
    }

    void processInsert() {
        string token = getCommand();
        bool err = false;
        if (queryList.empty()) {
            cerr << "Syntax Error: INSERT -> no arguments provided"<< endl;
            return;
        }
        if (!currentTableInstance) {
            cout << "Sys ERR: INSERT -> table not selected." << endl;
            queryList.clear();
            return;
        }
        while (!queryList.empty()) {
            string valuesToken = getCommand();
            // // Reconstruct a command string for insertRow.
            // string insertCommand = "INSERT(" + valuesToken + ")";
            if (currentTableInstance){
                currentTableInstance->insertRow(valuesToken);
            }
            if(!err){
                cout << "Response: Data inserted successfully into table."<< endl;
            }
        }
    }

    void processEnter() {
        // ENTER <database_name>
        if(!currentDatabase.empty() || !currentTable.empty()){
            cerr<<"Sys ERR: Enter -> cannot be use in table/database."<< endl;
            queryList.clear();
        }
        if (queryList.empty()) {
            cout << "Syntax Error: ENTER -> Database name expected." << endl;
            return;
        }
        string db_name = getCommand();
        if(checkExtraTokens()) return;
        try {
            if (fs::exists(db_name) && fs::is_directory(db_name)) {
                fs::current_path(db_name);
                currentDatabase = db_name;
                // currentTable = "";
                // if (currentTableInstance) { // here istance of currentTableInstance is deleted.
                //     delete currentTableInstance;
                //     currentTableInstance = nullptr;
                // }
            } else {
                cout << "Logic ERR: Database \"" << db_name << "\" does not exist. Create it using INIT command." << endl;
            }
        } catch (const fs::filesystem_error &e) {
            cerr << "Program Error: This shouldn't happen contact dev.\n" << e.what() << endl;
        }
    }

    void processChoose() {
        // CHOOSE <table_name>
        if (currentDatabase.empty()) {
            cout << "Sys ERR: Enter a database to use CHOOSE" << endl;
            queryList.clear();
            return;
        }
        if (!currentTable.empty()) {
            cout << "Sys ERR: CHOOSE -> cannot be used within a table." << endl;
            queryList.clear();
            return;
        }
        if (queryList.empty()) {
            cout << "Syntax Error: CHOOSE -> Table name expected." << endl;
            return;
        }
        string table_name = getCommand();
        if(checkExtraTokens()) return;
        try {
            string filename = table_name + ".csv";
            ifstream file(filename);
            if (file.good()) {
                file.close();
                currentTable = table_name;
                // if (currentTableInstance) {
                //     delete currentTableInstance;
                //     currentTableInstance = nullptr;
                // }
                currentTableInstance = new Table(table_name);
            } else {
                cerr << "Table \"" << table_name << "\" does not exist.Create it using MAKE command." << endl;
            }
        } catch (const fs::filesystem_error &e) {
            cerr << "Program Error: this shouldn't happen contact dev.\n" << e.what() << endl;
        }
    }

    void processClose() {
        if(checkExtraTokens()) return;
        if (currentTableInstance) {
            exitTable();
        }
        cout << "Response: Program Exited" << endl; // make sure to change the project name
        exitProgram = true;
    }

    void processExit() {
        if(checkExtraTokens()) return;
        move_up();
    }

    void processList() {
        if(checkExtraTokens()) return;
        if (currentDatabase == "" && currentTable == "") {
            listDatabases();
        }
        else if (currentDatabase != "" && currentTable == "") {
            listTables();
        }
        else if (currentDatabase != "" && currentTable != "") {
            cerr << "Sys ERR: LIST -> is not available in table." << endl;
            queryList.clear();
        }
    }

    void processShow() {
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
            cout << "Syntax Error: SHOW -> No table selected." << endl;
    }

    void processRollback() {
        if (currentTable.empty()){
            cout << "Sys ERR: No table selected for transaction rollback." << endl;
            return;
        }
        if (currentTableInstance)
            currentTableInstance->rollbackTransaction();
    }

    void processCommit() {
        if (currentTable.empty()){
            cout << "Sys ERR: No table selected for transaction commit." << endl;
            return;
        }
        if (currentTableInstance)
            currentTableInstance->commitTransaction();
        else
            cout << "Sys ERR: No table selected for transaction commit." << endl;
    }

public:
    // Constructor: simply copy the tokens.
    Parser(const list<string> &tokens) : queryList(tokens) {}
    string getCommand(){
        string cmd = queryList.front();
        queryList.pop_front();
        return cmd;
    }
    bool checkExtraTokens(){
        if (!queryList.empty()) {
            cout << "Syntax Error: unexpected token \""<< queryList.front() << "\"."<< endl;
            queryList.clear();
            return true;
        }
        return false;
    }
    // The parse method processes all tokens.
    void parse() {
        while (!queryList.empty()) {
            string query = getCommand();
            if (query == INIT) {
                processInit();
            }
            else if (query == MAKE) {
                processMake();
            }
            else if (query == ERASE) {
                processErase();
            }
            else if (query == CLEAN) {
                processClean();
            }
            else if (query == DEL) {
                processDel();
            }
            else if (query == CHANGE) {
                processChange();
            }
            else if (query == INSERT) {
                processInsert();
            }
            else if (query == ENTER) {
                processEnter();
            }
            else if (query == CHOOSE) {
                processChoose();
            }
            else if (query == CLOSE) {
                processClose();
            }
            else if (query == EXIT) {
                if(!currentTable.empty()){
                    exitTable();
                } else {
                    processExit();
                }  
            }
            else if (query == DESCRIBE) {
                processDescribe();
            }
            
            else if (query == LIST) {
                processList();
            }
            else if (query == SHOW) {
                processShow();
            }
            else if (query == ROLLBACK) {
                processRollback();
            }
            else if (query == COMMIT) {
                processCommit();
            }
            else {
                cout << "Unknown command: " << query << endl;
            }
        }
    }
    friend void checkExtraTokens();
    ~Parser() {
        // Destructor: clean up if needed.
    }
};

/*     Done    */
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
                    // word.push_back(ch);  /////////////////////////////  I REMOVED THE PRESERVE QUOTE IF ANY ERROR OCCURS WHILE PARSING CHECK THIS
                    break;
                case '\'':
                    s_quotation = !s_quotation;
                    // word.push_back(ch); /////////////////////////////  I REMOVED THE PRESERVE QUOTE IF ANY ERROR OCCURS WHILE PARSING CHECK THIS
                    break;
                case '|':
                case '=':
                case '<':
                case '>':
                case '*':
                case '!':
                    if (!s_quotation && !d_quotation) {
                        word.push_back(ch);
                    }
                    break;
                default:
                    if (!s_quotation && !d_quotation) {
                        // This is imported from <cctype> library, it is used to check whether a character is a-z,A-Z
                        if (isalpha(ch))
                            word.push_back(tolower(ch));
                        else if(isalnum(ch))
                            word.push_back(ch);
                        else {
                            cerr << ch << " is not expected."<< endl;
                            return {};
                        }

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
        cerr << "Mismatched parentheses in input." << endl;
    
// Comment out below lines, For debugging: print tokens
    for (const string &token : query) {
        cout << token << endl;
    }
    return query;
}



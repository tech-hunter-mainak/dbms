#include "utils.cpp"  
#include "table.cpp"


extern string fs_path;         // Root DBMS folder path
extern string currentDatabase; // Currently selected database name (empty if none)
extern string currentTable;    // Currently selected table name (empty if none)
extern bool exitProgram;  // Declaration of the global variable

// Global pointer to the current Table instance.
Table* currentTableInstance = nullptr;
void exitTable() {
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
            cout << "\033[32mres: Discarding changes.\033[0m" << endl;
        }
    }
    delete currentTableInstance;
    currentTableInstance = nullptr;
    currentTable = "";      // Clear current table context.
}
class Parser {
private:
    list<string> queryList;
    // Helper functions for each command:
    void processInit() {
        // INIT <database_name>
        string dbName = getCommand();
        checkExtraTokens();
        init_database(dbName);
    }
    void processMake() {
        // MAKE <table_name> [ ( <column_definitions> ) ]
        if(currentDatabase.empty() || !currentTable.empty()){
            throw logic_error("MAKE -> not used before entering a database / within a table .");
        }
        string tableName = getCommand();    
        // Set the global currentTable to the table name.
        if(tableName[0] == ' ' || tableName[tableName.size() - 1] == ' '){
            throw invalid_argument("Table name cannot start/end with <space>");
        }
        // Call the free function make_table with only the header definitions.
        make_table(queryList, tableName);
        // After creation, currentTable should be set by make_table.
        // Create a new Table instance to manage further operations.
        currentTable = tableName;
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
        string name = getCommand();
        checkExtraTokens();
        // At root level (no database selected): erase a database.
        if (currentDatabase.empty()) {
            eraseDatabase(name);
            if (currentDatabase == name)
                currentDatabase = "";
        }
        // Inside a database: erase a table.
        else {
            eraseTable(name);
            if (currentTable == name)
                currentTable = "";
            if (currentTableInstance) { //////////////    TABLE INSTANCE IS HERE
                delete currentTableInstance;
                currentTableInstance = nullptr;
            }
        }
    }
    void processClean() {
        if (currentTable.empty()) {
            throw logic_error("Please choose a table before cleaning.");
        } else {
            checkExtraTokens();
            if (currentTableInstance)
                currentTableInstance->cleanTable();
        }
    }

    void processDel() {
        if(currentTable.empty()){
            throw logic_error("Please choose a table before cleaning.");
        }
        if (queryList.empty()) {
            throw "syntax_error: missing commands.";
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
            if (currentTableInstance)
                currentTableInstance->deleteRowsByAdvancedConditions(conditionGroups);
        }
        else {
            // Otherwise, treat tokens as either row IDs or column names.
            vector<string> items;
            while (!queryList.empty()) {
                string token = getCommand();
                // If the token contains commas, split it.
                // size_t pos = 0;
                // while ((pos = token.find(',')) != string::npos) {
                //     string part = token.substr(0, pos);
                //     if (!part.empty())
                //         items.push_back(part);
                //     token = token.substr(pos + 1);
                // }
                if (!token.empty())
                    items.push_back(token);
                else
                    throw invalid_argument("Id (or) Column Name cannot be empty.");
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
                    throw invalid_argument("No record matches a Id (or) Column Name with value " + item + ".");
            }
        }
    }
    void processDescribe(){
        if (currentTable.empty()) {
            throw logic_error("DESCRIBE -> can only be used in table");
        }
        checkExtraTokens();
        if (currentTableInstance)
            currentTableInstance->describe();
    }
    void processChange() {
        // This branch supports two syntaxes:
            // Syntax 1: CHANGE <column_name> <old_value> TO <new_value> Where <condition>
            // Syntax 2: CHANGE <old_value> TO <new_value> Where <condition>
            if (!currentTableInstance) {
                throw logic_error("CHANGE -> No table selected.");
            }
            
            bool hasColumnSpecified = false;
            string colName, oldValue, newValue;
            string firstToken = getCommand();
            
            // If the next token is "TO", then it's Syntax 2 (no column specified).
            if (!queryList.empty() && queryList.front() == TO) {
                oldValue = firstToken;
                // Remove "TO"
                queryList.pop_front();
                newValue = getCommand();
            } else {
                // Otherwise, it's Syntax 1 (with column name).
                hasColumnSpecified = true;
                colName = firstToken;
                // Remove surrounding quotes if present.
                if (!colName.empty() && ((colName.front() == '"' && colName.back() == '"') || (colName.front() == '\'' && colName.back() == '\''))) {
                    colName = colName.substr(1, colName.size() - 2);
                }
                oldValue = getCommand();
                if (queryList.empty() || queryList.front() != TO) {
                    throw ("syntax_error: CHANGE -> missing/unexpected command TO.");
                }
                // Remove "TO"
                queryList.pop_front();
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
                } else {
                    throw "syntax_error: unexpected command \""+ token + ".";
                }
            }
            // Call the appropriate Table method.
            if (hasColumnSpecified)
                currentTableInstance->updateValueByCondition(colName, oldValue, newValue, conditionGroups);
            else
                currentTableInstance->updateValueByCondition(oldValue, newValue, conditionGroups);
    }

    void processInsert() {
        if (!currentTableInstance) {
            throw  logic_error("INSERT -> table not selected.");
        }
        if(queryList.empty()){
            throw ("syntax_error: missing commands.");
        }
        while (!queryList.empty()) {
            string valuesToken = getCommand();
            // if(trim(valuesToken).empty()) throw 
            // // Reconstruct a command string for insertRow.
            // string insertCommand = "INSERT(" + valuesToken + ")";
            if (currentTableInstance){
                currentTableInstance->insertRow(valuesToken);
            }
            cout << "\033[32mres: Data sucessfully inserted.\033[0m"<< endl;
        }
    }

    void processEnter() {
        // ENTER <database_name>
        if(!currentDatabase.empty() || !currentTable.empty()){
            throw logic_error("Enter -> cannot be used in table/database.");
        }
        string db_name = getCommand();
        fs::path db_path = fs::path(fs_path) / db_name;
        checkExtraTokens();
        try {
            if (fs::exists(db_path) && fs::is_directory(db_path)) {
                fs::current_path(db_path);
                currentDatabase = db_name;
                currentTable = "";
                if (currentTableInstance) { // here istance of currentTableInstance is deleted. if somehow it is still there
                    delete currentTableInstance;
                    currentTableInstance = nullptr;
                }
            } else {
                throw logic_error("Database \"" + db_name + "\" does not exist.");
            }
        } catch (const fs::filesystem_error &e) {
            string errorMessage = "program_error: " + string(e.what()) + ".";
            throw errorMessage;
        }
    }

    void processChoose() {
        // CHOOSE <table_name>
        if (currentDatabase.empty() || !currentTable.empty()) {
            throw logic_error("CHOOSE -> cannot be used in table/outside a database.");
        }
        string table_name = getCommand();
        checkExtraTokens();
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
                throw logic_error("table \"" + table_name + "\" does not exist.");
            }
        } catch (const fs::filesystem_error &e) {
            string errorMessage = "program_error: " + string(e.what()) + ".";
            throw errorMessage;
        }
    }

    void processClose() {
        checkExtraTokens();
        if (currentTableInstance) {
            exitTable();
        }
        exitProgram = true;
    }

    void processExit() {
        checkExtraTokens();
        move_up();
    }

    void processList() {
        checkExtraTokens();
        if (currentDatabase == "" && currentTable == "") {
            listDatabases();
        }
        else if (currentDatabase != "" && currentTable == "") {
            listTables();
        }
        else if (currentDatabase != "" && currentTable != "") {
            throw logic_error("LIST -> not available in table.");
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
        if (currentTableInstance)
            currentTableInstance->rollbackTransaction();
        else
            throw logic_error("No table selected for transaction ROLLBACK.");
    }

    void processCommit() {
        if (currentTableInstance)
            currentTableInstance->commitTransaction();
        else
            throw logic_error("No table selected for transaction COMMIT.");
    }

public:
    // Constructor: simply copy the tokens.
    Parser(const list<string> &tokens) : queryList(tokens) {}
    string getCommand(){
        if (queryList.empty()) {
            throw "syntax_error: missing commands.";
        }
        string cmd = queryList.front();
        queryList.pop_front();
        return cmd;
    }
    void checkExtraTokens(){
        if (!queryList.empty()) {
            throw ("syntax_error: unexpected command \""+ queryList.front()+"\".");
        }
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
                    if (!currentTable.empty()) {
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
                    throw ("syntax_error: unknown query " + query );
                }
        }
    }
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
                case '.':
                case '~':
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
                            string errorMessage = "syntax_error: ";
                            errorMessage.push_back(ch);
                            errorMessage += " is not expected.";
                            throw (errorMessage);
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
    if (inside_parentheses){
        throw ("syntax_error: Mismatched parentheses in input.");
    }
// Comment out below lines, For debugging: print tokens
    // for (const string &token : query) {
    //     cout << token << endl;
    // }
    return query;
}



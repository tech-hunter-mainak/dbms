// initiateData.cpp
#include "library.cpp"
#include "keywords.cpp"

// Global variables used for session context.
extern string fs_path;         // Root DBMS folder path
extern string currentDatabase; // Currently selected database name (empty if none)
extern string currentTable;    // Currently selected table name (empty if none)

//--------------------------------------------------------------------------------
// Database & Table Creation / Erasure Functions
//--------------------------------------------------------------------------------

// Creates a new database directory if it does not exist.
bool init_database(string name) {
    if (!fs::exists(name)) {
        if (fs::create_directory(name)) {
            cout << "Database created: " << name << endl;
            cout << "To access it, use: ENTER " << name << endl;
        } else {
            cerr << "Failed to initiate database! Try using a different name." << endl;
            return false;
        }
    } else {
        cout << "Database already exists! Use ENTER " << name << " to access." << endl;
        return false;
    }
    return true;
}

// Creates a new table by writing a CSV file with header definitions.
// The header is written in the format: 
//   id(INT)(PRIMARY_KEY),name(VARCHAR),age(INT)
// This format is used internally for data type validation and metadata tracking.
// When the table is later displayed via the SHOW command, only the column names
// (extracted from the header) are shown.
// Helper function: trim whitespace from both ends of a string.
string trimStr(const string &s) {
    size_t start = s.find_first_not_of(" \t");
    if(start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

// Helper function: Tokenize a column definition by spaces, preserving quoted substrings.
vector<string> tokenizeColumnDef(const string &colDef) {
    vector<string> tokens;
    bool inQuotes = false;
    string current;
    for (char ch : colDef) {
        if (ch == '"') {
            inQuotes = !inQuotes;
            // Append the quote to preserve it.
            current.push_back(ch);
            continue;
        }
        if (!inQuotes && isspace(ch)) {
            if (!current.empty()) {
                tokens.push_back(trimStr(current));
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty())
        tokens.push_back(trimStr(current));
    return tokens;
}

// New helper: Parse a single column definition.
// If the definition contains quoted text, the text within the first pair of quotes is taken as the column name.
// The remainder of the definition is then tokenized (by spaces) for datatype and constraints.
vector<string> parseColumnDefinition(const string &colDef) {
    size_t firstQuote = colDef.find('"');
    if (firstQuote != string::npos) {
        size_t secondQuote = colDef.find('"', firstQuote + 1);
        if (secondQuote != string::npos) {
            // Extract column name (including quotes).
            string colName = colDef.substr(firstQuote, secondQuote - firstQuote + 1);
            // Remove the quotes for the final column name.
            colName = colName.substr(1, colName.size() - 2);
            // Get the remainder of the string after the second quote.
            string remainder = colDef.substr(secondQuote + 1);
            vector<string> tokens;
            tokens.push_back(trimStr(colName));  // Use the quoted text as column name.
            // Tokenize the remainder by spaces.
            istringstream iss(remainder);
            string token;
            while (iss >> token) {
                tokens.push_back(token);
            }
            return tokens;
        }
    }
    // If no quotes are found, use the normal tokenization.
    return tokenizeColumnDef(colDef);
}

// Creates a new table by writing a CSV file with custom header definitions.
// The header is constructed by splitting the user-provided header string by commas,
// then processing each column definition using parseColumnDefinition().
// For each column, the header is written as: 
//    columnName(DATATYPE)(CONSTRAINT1)(CONSTRAINT2)...
// Every column must have at least a column name and a datatype.
void make_table(list<string> &queryList) {
    if (queryList.empty()) {
        cout << "MAKE: Table name expected." << endl;
        return;
    }
    // The first token is the table name.
    string tableName = queryList.front();
    queryList.pop_front();

    // Gather the header string (if provided).
    string headersToken = "";
    if (!queryList.empty()) {
        headersToken = queryList.front();
        queryList.pop_front();
        // If headersToken starts with '(' and doesn't end with ')', combine tokens until a closing ')' is found.
        if (!headersToken.empty() && headersToken.front() == '(' && headersToken.back() != ')') {
            while (!queryList.empty() && queryList.back().back() != ')') {
                headersToken += " " + queryList.front();
                queryList.pop_front();
            }
            if (!queryList.empty()) {
                headersToken += " " + queryList.front();
                queryList.pop_front();
            }
        }
        headersToken = trimStr(headersToken);
        if (!headersToken.empty() && headersToken.front() == '(' && headersToken.back() == ')') {
            headersToken = headersToken.substr(1, headersToken.size() - 2);
        }
    }
    
    // Open a new CSV file for the table.
    string filename = tableName + ".csv";
    ifstream file(filename);
    if (file.good()) {
        cout << "Table already exists: " << fs::absolute(filename).string() << endl;
        return;
    }
    file.close();

    ofstream newTable(filename);
    if (!newTable.is_open()) {
        cerr << "Failed to create table file!" << endl;
        return;
    }
    
    string finalHeader;
    if (!headersToken.empty()) {
        // Split the header string by commas.
        vector<string> colDefs;
        istringstream headerStream(headersToken);
        string colDef;
        while(getline(headerStream, colDef, ',')) {
            colDef = trimStr(colDef);
            if (!colDef.empty())
                colDefs.push_back(colDef);
        }
        // Process each column definition.
        vector<string> formattedCols;
        for (const auto &def : colDefs) {
            vector<string> tokens = parseColumnDefinition(def);
            if (tokens.size() < 2) {
                cout << "Invalid column definition (must have column name and datatype): " << def << endl;
                return;
            }
            // First token: column name.
            string colName = tokens[0];
            // Second token: datatype.
            string dataType = tokens[1];
            transform(dataType.begin(), dataType.end(), dataType.begin(), ::toupper);
            // Any additional tokens are constraints.
            string constraints = "";
            for (size_t i = 2; i < tokens.size(); i++) {
                string cons = tokens[i];
                transform(cons.begin(), cons.end(), cons.begin(), ::toupper);
                constraints += "(" + cons + ")";
            }
            // Build formatted column: columnName(DATATYPE)(constraint...)
            string formatted = colName + "(" + dataType + ")" + constraints;
            formattedCols.push_back(formatted);
        }
        // Join formatted columns with commas.
        for (size_t i = 0; i < formattedCols.size(); i++) {
            finalHeader += formattedCols[i];
            if (i != formattedCols.size() - 1)
                finalHeader += ",";
        }
    } else {
        // If no header provided, use a fixed default header.
        finalHeader = "ID(INT)(PRIMARY_KEY),NAME(VARCHAR),AGE(INT),DEPARTMENT(VARCHAR)";
    }
    
    // Write the header to the CSV file.
    newTable << finalHeader << "\n";
    cout << "Table created with headers: " << finalHeader << endl;
    newTable.close();
    
    // Set currentTable so that subsequent operations know a table is active.
    currentTable = tableName;
    cout << "Table file created at: " << fs::absolute(filename).string() << endl;
}


// Erases a database directory (and all its contents) if it exists.
bool eraseDatabase(const string &dbName) {
    if (fs::exists(dbName) && fs::is_directory(dbName)) {
        fs::remove_all(dbName);
        cout << "Database erased: " << fs::absolute(dbName).string() << endl;
        return true;
    } else {
        cout << "Database not found: " << dbName << endl;
        return false;
    }
}

// Erases a table by deleting its CSV file.
bool eraseTable(const string &tableName) {
    string filename = tableName + ".csv";
    ifstream file(filename);
    if (file.good()) {
        file.close();
        if (remove(filename.c_str()) == 0) {
            cout << "Table erased: " << fs::absolute(filename).string() << endl;
            return true;
        } else {
            cout << "Error erasing table: " << tableName << endl;
            return false;
        }
    } else {
        cout << "Table not found: " << tableName << endl;
        return false;
    }
}

//--------------------------------------------------------------------------------
// Listing & Navigation Functions
//--------------------------------------------------------------------------------

// Lists all databases (directories) in the root DBMS folder.
void listDatabases() {
    fs::path rootPath = fs_path;  // Root DBMS folder
    if (!fs::exists(rootPath)) {
        cout << "Root DBMS directory not found." << endl;
        return;
    }
    for (const auto &entry : fs::directory_iterator(rootPath)) {
        if (entry.is_directory()) {
            string dbName = entry.path().filename().string();
            int tableCount = 0;
            for (const auto &f : fs::directory_iterator(entry.path())) {
                // Count CSV files (tables), ignoring the metadata file.
                if (f.is_regular_file() && f.path().extension() == ".csv" &&
                    f.path().stem().string() != "table_metadata")
                    tableCount++;
            }
            cout << dbName << " - " << tableCount << " tb" << endl;
        }
    }
}

// Lists all tables (CSV files) in the current database directory.
void listTables() {
    fs::path dbPath = fs::current_path();  // Current directory is the selected database.
    for (const auto &entry : fs::directory_iterator(dbPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            string tableName = entry.path().stem().string();
            if (tableName == "table_metadata")
                continue;
            int rowCount = 0;
            ifstream file(entry.path());
            string line;
            bool header = true;
            while(getline(file, line)) {
                if (header) { header = false; continue; }
                if (!line.empty())
                    rowCount++;
            }
            file.close();
            cout << tableName << " - " << rowCount << " rows" << endl;
        }
    }
}

// Splits a list of tokens into separate queries based on the pipe ("|") symbol.
list<list<string>> splitQueries(const list<string>& tokens) {
    list<list<string>> queries;
    list<string> current;
    for (const auto &tok : tokens) {
        if (tok == "|") {
            if (!current.empty()) {
                queries.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(tok);
        }
    }
    if (!current.empty()) {
        queries.push_back(current);
    }
    return queries;
}

// Moves up one level in the directory hierarchy.
// If already at the root DBMS folder, terminates the program.
void move_up(){
    fs::path currentPath = fs::current_path();
    // If a table is active, clear table context.
    if(currentTable != ""){
        currentTable = "";
        cout << "Exited table context." << endl;
        return;
    }
    if(currentPath.string() == fs_path) {
        cout << "Exiting program." << endl;
        exit(0);  // Alternatively, set a global flag.
    } else {
        fs::current_path("../");
        currentPath = fs::current_path();
        if(currentPath.string() == fs_path){
            currentDatabase = "";
            currentTable = "";
            cout << "Now at top-level DBMS directory." << endl;
        } else {
            currentTable = "";
            cout << "Moved up. Current directory: " << currentPath.string() << endl;
        }
    }
}

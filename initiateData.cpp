#include "library.cpp"

// Global variables used for session context.
extern string fs_path;         // Root DBMS folder path
extern string currentDatabase; // Currently selected database name (empty if none)
extern string currentTable;    // Currently selected table name (empty if none)
extern bool exitProgram;


//--------------------------------------------------------------------------------
// Database & Table Creation / Erasure Functions
//--------------------------------------------------------------------------------

/* DONE */
// Creates a new database directory if it does not exist.
bool isValidDatabaseName(const string& name) {
    // Only allow alphabets (upper and lower case), numbers, and underscores.
    regex validPattern("^[A-Za-z0-9_]+$");
    return regex_match(name, validPattern);
}

bool init_database(string name) {
    if (!isValidDatabaseName(name)) {
        cerr << "Invalid database name! Only alphabets, numbers, and underscores are allowed." << endl;
        return false;
    }
    if (!fs::exists(name)) {
        if (fs::create_directory(name)) {
            cout << "Database created: " << name << endl;
            cout << "To access it, use: ENTER " << name << endl;
        } else {
            cerr << "Failed to initiate database! Try using a different name." << endl;
            return false;
        }
    } else {
        cout << "Database already exists! Use ENTER " << name << " to access it." << endl;
        return false;
    }
    return true;
}

// Helper function: trim whitespace from both ends of a string.
// This is actually not required can be removed.
string trimStr(const string &s) {
    size_t start = s.find_first_not_of(" \t"); // Finds the first character that is not a space or tab
    if(start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t"); // Finds the last such character that is not a space or tab
    return s.substr(start, end - start + 1); // ✅ The result includes the character at start_index,❌ But does NOT use an end_index.
}
bool isValidDataType(const string &dataType) {
    // Use dummy values for type checking
    if (dataType == "INT") return true;
    if (dataType == "BIINT") return true;
    if (dataType == "DOUBLE") return true;
    if (dataType == "BIDOUBLE") return true;
    if (dataType == "CHAR") return true;
    if (dataType == "VARCHAR" || dataType == "STRING") return true;
    if (dataType == "DATE") return true;
    if (dataType == "BOOL") return true;
    return false; // Unknown type
}

bool isValidConstraint(const string &constraint) {
    string upperCons = constraint;
    transform(upperCons.begin(), upperCons.end(), upperCons.begin(), ::toupper);
    if(upperCons == "PRIMARY_KEY") return true;
    if(upperCons == "NOT_NULL") return true;
    return false;
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
            cout<<"Table is erased permanently."<<endl;
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
bool eraseTable(const string &tableName, int) { // function overloading 
    string filename = tableName + ".csv";
    ifstream file(filename);
    if (file.good()) {
        file.close();
        if (remove(filename.c_str()) == 0) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

// Helper function: Tokenize a column definition by spaces, preserving quoted substrings.
vector<string> tokenizeColumnDef(const string &colDef) {
    vector<string> tokens;
    bool inQuotes = false;
    string current;
    for (char ch : colDef) {
        if (ch == '"') {
            inQuotes = !inQuotes;
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


/////////// THIS FUNCTION IS NOT NEEDED tokenizeColumnDef(colDef); above function does all the necessary things)
// New helper: Parse a single column definition.
// If the definition contains quoted text, the text within the first pair of quotes is taken as the column name.
// The remainder of the definition is then tokenized (by spaces) for datatype and constraints.
// vector<string> parseColumnDefinition(const string &colDef) {
//     size_t firstQuote = colDef.find('"');
//     if (firstQuote != string::npos) {
//         size_t secondQuote = colDef.find('"', firstQuote + 1);
//         if (secondQuote != string::npos) {
//             // Extract column name (including quotes).
//             string colName = colDef.substr(firstQuote, secondQuote - firstQuote + 1);
//             // Remove the quotes for the final column name.
//             colName = colName.substr(1, colName.size() - 2);
//             // Get the remainder of the string after the second quote.
//             string remainder = colDef.substr(secondQuote + 1);
//             vector<string> tokens;
//             tokens.push_back(trimStr(colName));  // Use the quoted text as column name.
//             // Tokenize the remainder by spaces.
//             istringstream iss(remainder);
//             string token;
//             while (iss >> token) {
//                 tokens.push_back(token);
//             }
//             return tokens;
//         }
        
//     }
//     // If no quotes are found, use the normal tokenization.
//     return tokenizeColumnDef(colDef);
// }

// Creates a new table by writing a CSV file with custom header definitions.
// The header is constructed by splitting the user-provided header string by commas,
// then processing each column definition using parseColumnDefinition().
// For each column, the header is written as: 
//    columnName(DATATYPE)(CONSTRAINT1)(CONSTRAINT2)...
// Every column must have at least a column name and a datatype.
void make_table(list<string> &queryList) {
    string headersToken = "";
    headersToken = queryList.front();
    queryList.pop_front();
    headersToken = trimStr(headersToken);

    string filename = currentTable + ".csv";
    ifstream file(filename); // make this file absolute if needed //////////////////////
    if (file.good()) {
        cout << "Table already exists with name " << filename << endl;
        return;
    }
    file.close();

    ofstream newTable(filename);
    if (!newTable.is_open()) {
        cerr << "Failed to create table file!" << endl;
        return;
    }

    string finalHeader;
    int primaryKeyIndex = -1;

    if (!headersToken.empty()) {
        vector<string> colDefs;
        istringstream headerStream(headersToken);
        /*istringstream is a class from the <sstream> header. 
            It allows you to treat a string like a stream, so you can extract values from it just like you’d read from cin or a file.
        */ 
        string colDef;
        while (getline(headerStream, colDef, ',')) {
            colDef = trimStr(colDef);
            // Reads from headerStream, one comma-separated value at a time. like this 
            // " id INT PRIMARY_KEY"
            // " name VARCHAR"
            // " age INT"
            // " department VARCHAR"
            if (!colDef.empty())
                colDefs.push_back(colDef);
        }

        vector<string> formattedCols;
        int index = 0;
        for (const auto &def : colDefs) {
            vector<string> tokens = tokenizeColumnDef(def);
            if (tokens.size() < 2) {
                cout << "Invalid column definition (must have column name and datatype): " << def << endl;
                return;
            }

            string colName = tokens[0];
            string dataType = tokens[1];
            transform(dataType.begin(), dataType.end(), dataType.begin(), ::toupper);

            if (!isValidDataType(dataType)) {
                cout << "Invalid data type for column '" << colName << "': " << dataType << endl;
                string path = fs::current_path().string();
                eraseTable(path + "/" + currentTable, 0);
                currentTable = "";
                return;
            }

            string constraints = "";
            bool isPrimaryKey = false;
            for (size_t i = 2; i < tokens.size(); i++) {
                string cons = tokens[i];
                transform(cons.begin(), cons.end(), cons.begin(), ::toupper);
                if (!isValidConstraint(cons)) {
                    cout << "Invalid constraint '" << cons << "' for column '" << colName << "'." << endl;
                    string path = fs::current_path().string();
                    eraseTable(path + "/" + currentTable, 0);
                    currentTable = "";
                    return;
                }

                if (cons == "PRIMARY_KEY") {
                    if (primaryKeyIndex != -1) {
                        cerr << "Error: Multiple PRIMARY_KEY definitions found. Only one PRIMARY_KEY is allowed." << endl;
                        string path = fs::current_path().string();
                        eraseTable(path + "/" + currentTable, 0);
                        currentTable = "";
                        return;
                    }
                    if (dataType != "INT") {
                        cerr << "Error: Only INT columns can be defined as PRIMARY_KEY. Column '" << colName << "' has type '" << dataType << "'." << endl;
                        string path = fs::current_path().string();
                        eraseTable(path + "/" + currentTable, 0);
                        currentTable = "";
                        return;
                    }
                    isPrimaryKey = true;
                    primaryKeyIndex = index;
                }

                constraints += "(" + cons + ")";
            }

            string formatted = colName + "(" + dataType + ")" + constraints;
            formattedCols.push_back(formatted);
            index++;
        }

        if (primaryKeyIndex == -1) {
            cerr << "Error: No PRIMARY_KEY defined. Please specify a PRIMARY_KEY for the table." << endl;
            return;
        }

        for (size_t i = 0; i < formattedCols.size(); i++) {
            finalHeader += formattedCols[i];
            if (i != formattedCols.size() - 1)
                finalHeader += ",";
        }
    }

    newTable << finalHeader << "\n";
    cout << "Table created with headers: " << finalHeader << endl;
    newTable.close();
    cout << "Table is Successfully Created with primary key column index: " << primaryKeyIndex << endl;
}

//--------------------------------------------------------------------------------
// Listing & Navigation Functions
//--------------------------------------------------------------------------------

// Lists all databases (directories) in the root DBMS folder.
void listDatabases() {
    fs::path rootPath = fs_path;  // Root DBMS folder
    if (!fs::exists(rootPath)) {
        cerr << "Sys ERR: Installation went wrong unistall and install again." << endl;
        
    }
    for (const auto &entry : fs::directory_iterator(rootPath)) {
        if (entry.is_directory()) {
            string dbName = entry.path().filename().string();
            if (!dbName.empty() && dbName[0] == '.')// ✅ Skips hidden directories (starting with '.')
                continue;
            int tableCount = 0;
            for (const auto &f : fs::directory_iterator(entry.path())) { // fs::directory_iterator loops through each file in the db-name
                string fileName = f.path().filename().string();
                // ✅ Skips hidden files
                if (!fileName.empty() && fileName[0] == '.')
                    continue;
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

/*     Done    */
// Splits a list of tokens into separate queries based on the pipe ("|") symbol.
list<list<string>> splitQueries(const list<string>& tokens) {
    list<list<string>> queries; // creates a 2d list.
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
void clearTableInstance(){
    currentTable = "";
    // delete currentTableInstance;
    // currentTableInstance = nullptr;
    return;
}
void move_up(){
    fs::path currentPath = fs::current_path();
    if(currentTable != ""){
        cout<< currentPath<< endl;
        clearTableInstance();
        cout << "Exited table " << endl;
        return;
    }
    if(currentPath.string() == fs_path) {
        cout << "Exiting program." << endl;
        cout<< currentPath<< endl;
        exitProgram = true;
    } else {
        fs::current_path("../");
        currentPath = fs::current_path();
        cout<< currentPath<< endl;
        if(currentPath.string() == fs_path){
            currentDatabase = "";
            cout << "Now at top-level." << endl;
        }
    }
}



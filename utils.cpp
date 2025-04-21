#include "library.cpp"

#include <stdexcept>
#include <string>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#define AES_BLOCK_SIZE 16
// Global variables used for session context.
extern string fs_path;         // Root DBMS folder path
extern string currentDatabase; // Currently selected database name (empty if none)
extern string currentTable;    // Currently selected table name (empty if none)
extern bool exitProgram;
extern string aesKey;
//--------------------------------------------------------------------------------
// Database & Table Creation / Erasure Functions
//--------------------------------------------------------------------------------

string trimStr(const string &s);
/* DONE */
// Creates a new database directory if it does not exist.
bool isValidDatabaseName(const string& name) {
    // Only allow alphabets (upper and lower case), numbers, and underscores.
    regex validPattern("^[A-Za-z0-9_]+$");
    return regex_match(name, validPattern);
}
void removeTableMetadataEntry(const string &tr,const string &metaFileName = "table_metadata.txt") {
    // Construct the full path to metadata file.
    string metaFilePath = (fs::path(fs_path) / currentDatabase / metaFileName).string();

    // Read all lines from the metadata file.
    vector<string> lines;
    ifstream metaIn(metaFilePath);
    if (!metaIn.is_open()) {
        // File might not exist (or be created later), so nothing to remove.
        return;
    }
    string line;
    while (getline(metaIn, line)) {
        // Use a stringstream to extract the first token (table name).
        istringstream iss(line);
        string token;
        iss >> token;
        // Only keep lines that do NOT refer to the table we are erasing.
        if (token != tr) {
            lines.push_back(line);
        }
    }
    metaIn.close();

    // Rewrite the metadata file without the removed table.
    ofstream metaOut(metaFilePath);
    if (!metaOut.is_open()) {
        // If we cannot open the file for writing, just return.
        return;
    }
    for (const auto &l : lines) {
        metaOut << l << "\n";
    }
    metaOut.close();
}

void init_database(string name) {
    if (!isValidDatabaseName(name)) {
        throw logic_error("Invalid database name! Only alphabets, numbers, and underscores are allowed.");
    }
    fs::path db_path = fs::path(fs_path) / name;
    if (!fs::exists(db_path)) {
        if (fs::create_directory(db_path)) {
            // cout << "Database created: " << name << endl;
            // cout << "To access it, use: ENTER " << name << endl;
        } else {
            throw ("program_error: Failed to initiate database.");
        }
    } else {
        throw  logic_error("Database already exists! Use ENTER " + name + " to access it.");
    }
}
std::string sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(str.c_str()), str.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

    return ss.str();
}
std::string sha256WithSalt(const std::string& input, const std::string& salt) {
    const int rounds = 10;
    // Initial hash: concatenate password and salt
    std::string current = salt + input + salt;
    // Do the first hash
    current = sha256(current);
    
    // Do the remaining rounds: each round concatenates the previous hash and salt
    for (int i = 2; i <= rounds; i++) {
        if( i % 2 == 0){
            current = sha256(salt + current);
        } else {
            current = sha256(current + salt);
        }
        
    }
    return current;
}
// Encrypts plainText using AES-256-CBC.
// The 'key' must be 32 bytes (256 bits). 'ivOut' is set to the random IV.
std::string aesEncrypt(const std::string &plainText, std::string &ivOut) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create encryption context");

    // Generate a random IV.
    ivOut.resize(AES_BLOCK_SIZE, '\0');
    if (!RAND_bytes(reinterpret_cast<unsigned char*>(&ivOut[0]), AES_BLOCK_SIZE))
        throw std::runtime_error("Failed to generate IV");

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
                                reinterpret_cast<const unsigned char*>(aesKey.c_str()),
                                reinterpret_cast<const unsigned char*>(ivOut.c_str())))
        throw std::runtime_error("Encryption initialization failed");

    std::string cipherText;
    cipherText.resize(plainText.size() + AES_BLOCK_SIZE);
    int outLen1 = 0;
    if (1 != EVP_EncryptUpdate(ctx,
                               reinterpret_cast<unsigned char*>(&cipherText[0]),
                               &outLen1,
                               reinterpret_cast<const unsigned char*>(plainText.c_str()),
                               plainText.size()))
        throw std::runtime_error("Encryption update failed");

    int outLen2 = 0;
    if (1 != EVP_EncryptFinal_ex(ctx,
                                 reinterpret_cast<unsigned char*>(&cipherText[0]) + outLen1,
                                 &outLen2))
        throw std::runtime_error("Encryption finalization failed");

    EVP_CIPHER_CTX_free(ctx);
    cipherText.resize(outLen1 + outLen2);
    return cipherText;
}

// Decrypts cipherText (which was encrypted using AES-256-CBC) using the given key and iv.
std::string aesDecrypt(const std::string &cipherText, const std::string &iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create decryption context");

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
                                reinterpret_cast<const unsigned char*>(aesKey.c_str()),
                                reinterpret_cast<const unsigned char*>(iv.c_str())))
        throw std::runtime_error("Decryption initialization failed");

    std::string plainText;
    plainText.resize(cipherText.size());
    int outLen1 = 0;
    if (1 != EVP_DecryptUpdate(ctx,
                               reinterpret_cast<unsigned char*>(&plainText[0]),
                               &outLen1,
                               reinterpret_cast<const unsigned char*>(cipherText.c_str()),
                               cipherText.size()))
        throw std::runtime_error("Decryption update failed");

    int outLen2 = 0;
    if (1 != EVP_DecryptFinal_ex(ctx,
                                 reinterpret_cast<unsigned char*>(&plainText[0]) + outLen1,
                                 &outLen2))
        throw std::runtime_error("Decryption finalization failed");

    EVP_CIPHER_CTX_free(ctx);
    plainText.resize(outLen1 + outLen2);
    return plainText;
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
    if (dataType == "BIGINT") return true;
    if (dataType == "DOUBLE") return true;
    if (dataType == "BIGDOUBLE") return true;
    if (dataType == "CHAR") return true;
    if (dataType == "VARCHAR" || dataType == "STRING") return true;
    if (dataType == "DATE") return true;
    if (dataType == "BOOL") return true;
    return false; // Unknown type
}

bool isValidConstraint(const string &constraint) {
    string upperCons = constraint;
    transform(upperCons.begin(), upperCons.end(), upperCons.begin(), ::toupper);
    if(upperCons == "PRIMARY" || upperCons == "NOT_NULL" || upperCons == "UNIQUE" || upperCons == "AUTO_INCREMENT" || upperCons == "DEFAULT") return true;
    return false;
}
// Erases a database directory (and all its contents) if it exists.
void eraseDatabase(const string &dbName) {
    fs::path db_path = fs::path(fs_path) / dbName; 
    if (fs::exists(db_path) && fs::is_directory(db_path)) {
        fs::remove_all(db_path);
        // cout << "Database erased: " << fs::absolute(db_path).string() << endl; /////////- --- - - - - - ----- - - - --     ----------------    - - - ->>>> comment out this line
    } else {
        throw logic_error("Database not found: " + dbName);
    }
}

// Erases a table by deleting its CSV file.
void eraseTable(const string &tableName) {
    string filename = (fs::path(fs_path) / currentDatabase / (tableName + ".bin")).string();
    ifstream file(filename);
    if (file.good()) {
        file.close();
        if (remove(filename.c_str()) == 0) {
            cout<<"\033[32mres: Table erased permanently.\033[0m"<<endl;
        } else {
            throw ("program_error: could not erase " + tableName + '.');
        }
    } else {
        throw logic_error("Table not found: " + tableName);
    }
}
bool eraseTable(const string &tableName, int) { // function overloading 
    string filename = tableName + ".bin";
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
        if (ch == '"' || ch == '\'') {
            inQuotes = !inQuotes;
            continue;
        }
        if (!inQuotes && isspace(ch)) {
            if (!current.empty()) {
                // cout << current << endl;
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
// Creates a new table by writing a CSV file with custom header definitions.
// The header is constructed by splitting the user-provided header string by commas,
// then processing each column definition using parseColumnDefinition().
// For each column, the header is written as: 
//    columnName(DATATYPE)(CONSTRAINT1)(CONSTRAINT2)...
// Every column must have at least a column name and a datatype.
// Make sure you have included necessary headers and defined aesEncrypt as shown previously.

// The updated make_table function now takes a key as an argument.
void make_table(list<string> &queryList, const string tableName) {
    // Get the column definitions from the query.
    string headersToken = queryList.front();
    if (trimStr(headersToken).empty()) 
        throw ("syntax_error: Empty Column Definations");
    queryList.pop_front();
    
    headersToken = trimStr(headersToken);

    string filename = tableName + ".bin";
    ifstream file(filename);
    if (file.good()) {
        throw logic_error("Table already exists with name " + filename);
    }
    file.close();

    // Build the final CSV header row.
    string finalHeader;
    int primaryKeyIndex = -1;

    vector<string> formattedCols;
    if (!headersToken.empty()) {
        vector<string> colDefs;
        istringstream headerStream(headersToken);
        string colDef;
        while (getline(headerStream, colDef, ',')) {
            colDef = trimStr(colDef);
            if (!colDef.empty())
                colDefs.push_back(colDef);
        }
        
        int index = 0;
        for (const auto &def : colDefs) {
            vector<string> tokens = tokenizeColumnDef(def);
            if (tokens.size() < 2) {
                throw invalid_argument("Column must have a name and datatype: \"" + def + "\".");
            }
            string colName = tokens[0];
            string dataType = tokens[1];
            transform(dataType.begin(), dataType.end(), dataType.begin(), ::toupper);
            if (!isValidDataType(trimStr(dataType))) {
                throw invalid_argument("Invalid data type for column \"" + colName + "\" : " + dataType);
            }
            
            string constraints = "";
            bool isPrimaryKey = false;
            unordered_set<string> consSet;
            for (size_t i = 2; i < tokens.size(); i++) {
                string cons = tokens[i];
                transform(cons.begin(), cons.end(), cons.begin(), ::toupper);
                if (cons == "DEFAULT") {
                    if (consSet.count(cons)) {
                        throw ("WARNING: Multiple " + cons + " definitions found.");
                    }
                    i += 1;
                    if (i < tokens.size()) {
                        string defaultValue = tokens[i];
                        consSet.insert(cons);
                        constraints += "(" + cons + '#' + defaultValue + ")";
                        continue;
                    } else {
                        throw invalid_argument("No default value mentioned.");
                    }
                }
                if (!isValidConstraint(cons)) {
                    throw invalid_argument("Invalid constraint for column \"" + colName + "\" : " + cons);
                }
                if (cons == "PRIMARY") {
                    if (primaryKeyIndex != -1) {
                        throw invalid_argument("Multiple PRIMARY definitions found. Only one PRIMARY is allowed.");
                    }
                    if (dataType != "INT" && dataType != "BIGINT") {
                        throw invalid_argument("Only INT/BIGINT columns can be defined as PRIMARY. Column \"" + colName + "\" has type " + dataType + ".");
                    }
                    isPrimaryKey = true;
                    primaryKeyIndex = index;
                }
                if (consSet.count(cons)) {
                    throw ("WARNING: Multiple " + cons + " definitions found.");
                } else {
                    consSet.insert(cons);
                    constraints += "(" + cons + ")";
                }
            }
            consSet.clear();
            // Reconstruct the column in CSV format: name(dataType)(constraint1)...(constraintN)
            string formatted = colName + "(" + dataType + ")" + constraints;
            formattedCols.push_back(formatted);
            index++;
        }
        // If no PRIMARY key was provided, add one automatically.
        if (primaryKeyIndex == -1) {
            primaryKeyIndex = 0;
            formattedCols.insert(formattedCols.begin(), "self_pk(INT)(PRIMARY)");
        }
        // Join all formatted columns with a comma delimiter.
        for (size_t i = 0; i < formattedCols.size(); i++) {
            finalHeader += formattedCols[i];
            if (i != formattedCols.size() - 1)
                finalHeader += ",";
        }
    }
    
    // Create the CSV string (only header row for now).
    string csvData = finalHeader + "\n";

    // --- AES Encryption Step ---
    // Instead of using the simple XOR encryption, now use aesEncrypt.
    // aesEncrypt accepts the plaintext (csvData), the AES key, and returns the ciphertext,
    // while also setting the IV (initialization vector) passed by reference.
    std::string iv;  // IV will be generated inside aesEncrypt.
    std::string cipherText = aesEncrypt(csvData, iv);
    
    // --- Write Encrypted Data to the Binary File ---
    // We will write the IV first (fixed size: AES_BLOCK_SIZE) and then the ciphertext.
    ofstream newTable(filename, ios::binary);
    if (!newTable.is_open()) {
        throw ("program_error: Failed to create table!");
    }
    newTable.write(iv.c_str(), AES_BLOCK_SIZE);
    newTable.write(cipherText.c_str(), cipherText.size());
    newTable.close();
    
    cout << "\033[32mres: Table Created Successfully.\033[0m" << endl;
}



//--------------------------------------------------------------------------------
// Listing & Navigation Functions
//--------------------------------------------------------------------------------
void processHelp() {
    // ANSI color codes
    const string TIT   = "\033[1;36m";
    const string HDR   = "\033[1;33m";
    const string CMD   = "\033[1;32m";
    const string ARG   = "\033[0;37m";
    const string RESET = "\033[0m";

    const int CMD_WIDTH = 15;  // tightened from 30 → 15

    cout << "\n" << TIT << "============================ qiloDB Help ==========================" << RESET << "\n\n";

    auto printLine = [&](const string &command, const string &desc) {
        cout << "  "
             << CMD << left << setw(CMD_WIDTH) << command << RESET
             << " - " << desc << "\n";
    };

    cout << HDR << "Database Commands:" << RESET << "\n";
    printLine("init <db>",      "Create a new database.");
    printLine("erase <db>",     "Delete a database (only at root).");
    printLine("enter <db>",     "Select/use a database.");
    printLine("exit",                 "Exit context (table→db→root).");
    cout << "\n";

    cout << HDR << "Table Commands:" << RESET << "\n";
    printLine("make <table>(...)",    "Create a new table with columns.");
    cout << "       " << ARG << "Syntax: make users(id INT PRIMARY, name VARCHAR)" << RESET << "\n";
    printLine("choose <table>",       "Open a table in current database.");
    printLine("erase <table>",        "Delete a table (inside a DB).");
    printLine("clean",                "Remove all rows in the current table.");
    cout << "       " << ARG << "* Must choose a table first." << RESET << "\n\n";

    cout << HDR << "Data Operations:" << RESET << "\n";
    printLine("insert <v1>,<v2>",     "Add a new row to the table.");
    printLine("del <id(s)>",          "Delete row(s) by primary key.");
    printLine("del column(s)",        "Delete one or more columns.");
    printLine("del where <conds>",    "Delete rows matching condition.");
    printLine("change ... TO ...",    "Update values under conditions.");
    cout << "     " << ARG << "* CHANGE <colName(s)> <old_value> TO <new_value>" << RESET << "\n";
    cout << "     " << ARG << "* CHANGE <colName(s)> <old_value> TO <new_value> Where [(cond(s))]" << RESET << "\n\n";

    cout << HDR << "Query & Display:" << RESET << "\n";
    printLine("describe",             "Show current table schema.");
    printLine("list",                 "List databases or tables.");
    printLine("show",                 "Display table data (head, limit, etc.).");
    cout << "     " << ARG << "* show head - first 5 rows\n";
    cout << "     " << ARG << "* show limit N - first N rows\n";
    cout << "     " << ARG << "* show limit ~N - last N rows\n";
    cout << "     " << ARG << "* show <cols> [where/like]\n\n";

    cout << HDR << "Transactions & Misc:" << RESET << "\n";
    printLine("rollback",             "Undo unsaved changes.");
    printLine("commit",               "Save changes to disk.");
    printLine("close",                "Close table and return to database.");
    printLine("help",                 "Show this help screen.");
    cout << "\n" << TIT << "==================================================================" << RESET << "\n\n";
}
// Lists all databases (directories) in the root DBMS folder.
void listDatabases() {
    fs::path rootPath = fs::path(fs_path);  // Root DBMS folder
    if (!fs::exists(rootPath)) {
        throw ("program_error: Installation went wrong unistall and install again.");
        
    }
    bool databaseFound = false;
    for (const auto &entry : fs::directory_iterator(rootPath)) {
        if (entry.is_directory()) {
            string dbName = entry.path().filename().string();
            if (!dbName.empty() && dbName[0] == '.')// ✅ Skips hidden directories (starting with '.')
                continue;
            int tableCount = 0;
            databaseFound = true;
            for (const auto &f : fs::directory_iterator(entry.path())) { // fs::directory_iterator loops through each file in the db-name
                string fileName = f.path().filename().string();
                // ✅ Skips hidden files
                if (!fileName.empty() && fileName[0] == '.')
                    continue;
                // Count CSV files (tables), ignoring the metadata file.
                if (f.is_regular_file() && f.path().extension() == ".bin" &&
                    f.path().stem().string() != "table_metadata")
                    tableCount++;
            }
            cout << dbName << " - " << tableCount << " tb" << endl;
        }
    }
    if(!databaseFound) cout<<"\033[31mEmpty\033[0m"<<endl;
}

// Lists all tables (CSV files) in the current database directory.
void listTables() {
    fs::path dbPath = fs::path(fs_path) / currentDatabase;  // Current directory is the selected database.
    fs::path metaPath = dbPath / "table_metadata.txt";

    // Check if the table_metadata.txt file exists.
    if (fs::exists(metaPath) && fs::is_regular_file(metaPath)) {
        ifstream metaFile(metaPath);
        if (!metaFile.is_open()) {
            // throw ("program_error: unable to open table_metadata.txt");
        }
        string line;
        bool tableFound = false;
        // Read and log each non-empty line from table_metadata.txt.
        while (getline(metaFile, line)) {
            if (!line.empty()) {
                cout << line << endl;
                tableFound = true;
            }
        }
        metaFile.close();
        // If metadata exists but is empty, log "no tables".
        if (!tableFound) {
            cout << "\033[31mEmpty Database\033[0m" << endl;
        }
    }else {
        cout << "\033[31mEmpty Database\033[0m" << endl;
    }
    // else {  // If table_metadata.txt does not exist, fallback to listing CSV files.
    //     bool tableFound = false;
    //     for (const auto &entry : fs::directory_iterator(dbPath)) {
    //         if (entry.is_regular_file() && entry.path().extension() == ".csv") {
    //             string tableName = entry.path().stem().string();
    //             // Skip any CSV whose stem is "table_metadata".
    //             if (tableName == "table_metadata")
    //                 continue;
    //             int rowCount = 0;
    //             ifstream file(entry.path());
    //             string line;
    //             bool header = true;
    //             while(getline(file, line)) {
    //                 if (header) {
    //                     header = false;
    //                     continue;
    //                 }
    //                 if (!line.empty()){
    //                     tableFound = true;
    //                     rowCount++;
    //                 }
    //             }
    //             file.close();
    //             cout << tableName << " - " << rowCount << " rows" << endl;
    //         }
    //         if (!tableFound) {
    //             cout << "\033[31mEmpty Database\033[0m" << endl;
    //         }
    //     }
    // }
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
            } else {
                throw ("syntax_error: Query cannot be empty.");
            }
        } else {
            current.push_back(tok);
        }
    }
    if (!current.empty()) {
        queries.push_back(current);
    }else {
        throw ("syntax_error: Query cannot be empty.");
    }
    return queries;
}
// Moves up one level in the directory hierarchy.
// If already at the root DBMS folder, terminates the program.
void move_up(){
    fs::path currentPath = fs::current_path();
    if(currentPath.string() == fs_path) {
        exitProgram = true;
    } else {
        fs::current_path("../");
        currentPath = fs::current_path();
        if(currentPath.string() == fs_path){
            currentDatabase = "";
        }
    }
}



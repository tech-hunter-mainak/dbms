#include "library.cpp"

using namespace std;
namespace fs = std::filesystem;

string fs_path = "";
bool exitProgram = false;
int columnWidth = 15;
// Global context variables
string currentDatabase = "";
string currentTable = "";

class Row
{
private:
    string id;
    vector<string> values;

public:
    // Default constructor
    Row() : id(""), values() {}
    // Parameterized constructor
    Row(string id, vector<string> values) : id(id), values(values) {}
    ~Row() {}

    // Give friend access to printData and show.
    friend void printData();
    friend void show(const string &params);
    friend void updateValueByCondition(const string &colName, const string &oldValue, const string &newValue);
    friend void commitTransaction(const string &tableName);
};

// Global data structures.
unordered_map<string, Row> dataMap;
vector<string> headers;
vector<string> rowOrder;  // NEW: Keeps row IDs in CSV (insertion) order.
string FILENAME = "data.csv";  // Current table file name

vector<string> extractValues(const string &command)
{
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
        // Trim spaces and remove quotes.
        item.erase(remove(item.begin(), item.end(), '"'), item.end());
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        values.push_back(item);
    }
    return values;
}


// Updated function: deleteRow now removes a row from the in‑memory dataMap.
void deleteRow(const string &id)
{
    auto it = dataMap.find(id);
    if (it != dataMap.end())
    {
        dataMap.erase(it);
        // Remove from rowOrder vector.
        rowOrder.erase(remove(rowOrder.begin(), rowOrder.end(), id), rowOrder.end());
        cout << "Row deleted successfully from in-memory data." << endl;
    }
    else
    {
        cout << "ID not found in in-memory data." << endl;
    }
}

// Updated function: updateValue now updates the in‑memory dataMap.
void updateValueByCondition(const string &colName, const string &oldValue, const string &newValue)
{
    int colIndex = -1;
    for (int i = 0; i < headers.size(); i++) {
        if (headers[i] == colName) {
            colIndex = i;
            break;
        }
    }
    if (colIndex == -1) {
        cout << "Column \"" << colName << "\" not found." << endl;
        return;
    }
    // Prevent updating primary key column (assuming header[0] is primary key).
    if (colIndex == 0) {
        cout << "Primary key column cannot be updated." << endl;
        return;
    }
    int updateCount = 0;
    // Iterate over all rows and update matching rows.
    for (auto &pair : dataMap) {
        // Adjust index: row.values index for header i corresponds to i-1.
        int index = colIndex - 1;
        if (index < pair.second.values.size() && pair.second.values[index] == oldValue) {
            pair.second.values[index] = newValue;
            updateCount++;
        }
    }
    if (updateCount > 0)
        cout << updateCount << " row(s) updated successfully." << endl;
    else
        cout << "No rows found with " << colName << " = " << oldValue << endl;
}
// Updated function: insertRow now adds a new row to the dataMap.
void insertRow(const string &command)
{
    vector<string> values = extractValues(command);
    if (values.empty()) {
        cout << "Error: No values extracted for insertion." << endl;
        return;
    }
    // The first value is assumed to be the row ID.
    string id = values[0];
    if (dataMap.find(id) != dataMap.end()) {
        cout << "Row with ID " << id << " already exists. Skipping insertion for this record." << endl;
        return;
    }
    // Remaining values are the row data.
    vector<string> rowValues(values.begin() + 1, values.end());
    dataMap[id] = Row(id, rowValues);
    rowOrder.push_back(id);  // Record the insertion order.
    cout << "Data inserted successfully into in-memory data." << endl;
}

// Function to delete all rows (clearing both dataMap and rowOrder).
void deleteAllExceptHeader()
{
    dataMap.clear();
    rowOrder.clear();
    cout << "All rows deleted from in-memory data (header preserved)." << endl;
}
// Updated function: cleanTable clears all rows from the dataMap (keeping headers intact).
void cleanTable()
{
    dataMap.clear();
    rowOrder.clear();
    cout << "Table cleaned in memory: " << currentTable << endl;
}

void retrieveData(string FILENAME_) {
    dataMap.clear();
    rowOrder.clear();
    headers.clear();
    ifstream file(FILENAME_);
    if (!file.is_open())
    {
        cout << "Error opening file!\n";
        return;
    }
    string line;
    bool isHeader = true;
    while (getline(file, line))
    {
        stringstream ss(line);
        vector<string> rowValues;
        string value;
        while (getline(ss, value, ',')) {
            rowValues.push_back(value);
        }
        if (isHeader) {
            headers = rowValues;
            isHeader = false;
        } else {
            if (!rowValues.empty()) {
                string id = rowValues[0];
                dataMap[id] = Row(id, vector<string>(rowValues.begin() + 1, rowValues.end()));
                rowOrder.push_back(id);  // Save the order as in the CSV.
            }
        }
    }
    file.close();
    cout << "Data loaded into memory.\n";
}

void printData()
{
    if (headers.empty())
    {
        cout << "No data to display.\n";
        return;
    }
    // Print header row.
    for (const auto &header : headers) {
        cout << setw(columnWidth) << left << header;
    }
    cout << "\n" << string(headers.size() * columnWidth, '-') << "\n";
    // Print data rows in CSV order.
    for (const auto &id : rowOrder) {
        auto it = dataMap.find(id);
        if (it != dataMap.end()) {
            cout << setw(columnWidth) << left << it->first;
            for (const auto &val : it->second.values) {
                cout << setw(columnWidth) << left << val;
            }
            cout << "\n";
        }
    }
}

bool init_database(string name) {
    if (!fs::exists(name)) {
        if (fs::create_directory(name)) {
            cout << "Directory created: " << name << endl;
        }
        else
        {
            cerr << "Failed to create directory!" << endl;
            return false;
        }
    }
    else
    {
        cout << "Directory already exists!" << endl;
        return false;
    }
    return true;
}

void use(string name, string type) {
    if (type == "dir") {
        fs::current_path(name);
        cout << "Current working directory: " << fs::current_path() << endl;
    } else {
        retrieveData(name + ".csv");
    }
}

void show(const string &params = "") {
    // Tokenize the input parameters.
    istringstream iss(params);
    vector<string> tokens;
    string token;
    while (iss >> token)
        tokens.push_back(token);

    // For debugging: print tokens if needed.
    // for (const auto &tok : tokens) cout << tok << endl;

    if (tokens.empty()) {
        cout << "No arguments provided for show command.\n";
        return;
    }

    // Function to print header row.
    auto printHeader = [&]() {
        for (const auto &hdr : headers)
            cout << setw(columnWidth) << left << hdr;
        cout << "\n" << string(headers.size() * columnWidth, '-') << "\n";
    };

    // "SHOW *" command (with optional filtering)
    if (tokens[0] == "*") {
        // Handle "SHOW * WHERE ..." filtering.
        if (tokens.size() > 1 && tokens[1] == "WHERE") {
            string colName, value;
            if (tokens.size() == 3) {
                size_t pos = tokens[2].find('=');
                if (pos == string::npos) {
                    cout << "Invalid criteria format. Expected COLUMN=VALUE.\n";
                    return;
                }
                colName = tokens[2].substr(0, pos);
                value = tokens[2].substr(pos + 1);
            } else if (tokens.size() >= 5 && tokens[3] == "=") {
                colName = tokens[2];
                value = tokens[4];
            } else {
                cout << "Invalid criteria format.\n";
                return;
            }
            // Remove leading and trailing quotes from colName, if present.
            if (!colName.empty() && colName.front() == '"' && colName.back() == '"') {
                colName = colName.substr(1, colName.size() - 2);
            }
            int colIndex = -1;
            for (int i = 0; i < headers.size(); i++) {
                if (headers[i] == colName) {
                    colIndex = i;
                    break;
                }
            }
            if (colIndex == -1) {
                cout << "Column `" << colName << "` not found.\n";
                return;
            }
            printHeader();
            // Iterate over rows in CSV order and display rows matching the criteria.
            for (const auto &id : rowOrder) {
                auto it = dataMap.find(id);
                if (it != dataMap.end()) {
                    bool match = false;
                    if (colIndex == 0)
                        match = (it->first == value);
                    else if ((colIndex - 1) < it->second.values.size())
                        match = (it->second.values[colIndex - 1] == value);
                    if (match) {
                        cout << setw(columnWidth) << left << it->first;
                        for (const auto &val : it->second.values)
                            cout << setw(columnWidth) << left << val;
                        cout << "\n";
                    }
                }
            }
        } else {
            // "SHOW *" without filtering: display all rows in CSV order.
            printHeader();
            for (const auto &id : rowOrder) {
                auto it = dataMap.find(id);
                if (it != dataMap.end()) {
                    cout << setw(columnWidth) << left << it->first;
                    for (const auto &val : it->second.values)
                        cout << setw(columnWidth) << left << val;
                    cout << "\n";
                }
            }
        }
    }
    // "SHOW HEAD": Display header and top defaultLimit rows.
    else if (tokens[0] == "HEAD") {
        int defaultLimit = 5;  // Adjust the default as needed.
        printHeader();
        int count = 0;
        for (const auto &id : rowOrder) {
            if (count >= defaultLimit) break;
            auto it = dataMap.find(id);
            if (it != dataMap.end()) {
                cout << setw(columnWidth) << left << it->first;
                for (const auto &val : it->second.values)
                    cout << setw(columnWidth) << left << val;
                cout << "\n";
                count++;
            }
        }
    }
    // "SHOW LIMIT" commands.
    else if (tokens[0] == "LIMIT") {
        if (tokens.size() < 2) {
            cout << "Missing number for LIMIT command.\n";
            return;
        }
        bool fromBottom = false;
        string numToken = tokens[1];
        if (numToken[0] == '~') {
            fromBottom = true;
            numToken = numToken.substr(1);
        }
        int limit = 0;
        try {
            limit = stoi(numToken);
        } catch (const exception &e) {
            cout << "Invalid number for LIMIT command.\n";
            return;
        }
        printHeader();
        if (limit <= 0) {
            cout << "Limit must be a positive integer.\n";
            return;
        }
        if (!fromBottom) {
            int count = 0;
            for (const auto &id : rowOrder) {
                if (count >= limit) break;
                auto it = dataMap.find(id);
                if (it != dataMap.end()) {
                    cout << setw(columnWidth) << left << it->first;
                    for (const auto &val : it->second.values)
                        cout << setw(columnWidth) << left << val;
                    cout << "\n";
                    count++;
                }
            }
        } else {
            int total = rowOrder.size();
            int start = max(0, total - limit);
            for (int i = start; i < total; i++) {
                auto it = dataMap.find(rowOrder[i]);
                if (it != dataMap.end()) {
                    cout << setw(columnWidth) << left << it->first;
                    for (const auto &val : it->second.values)
                        cout << setw(columnWidth) << left << val;
                    cout << "\n";
                }
            }
        }
    }
    // Otherwise, assume the command is "SHOW <column name>".
    else {
        string colName = tokens[0];
        int colIndex = -1;
        for (int i = 0; i < headers.size(); i++) {
            if (headers[i] == colName) {
                colIndex = i;
                break;
            }
        }
        if (colIndex == -1) {
            cout << "Column \"" << colName << "\" not found.\n";
            return;
        }
        cout << setw(columnWidth) << left << colName << "\n";
        cout << string(columnWidth, '-') << "\n";
        // Display the given column for each row in CSV order.
        for (const auto &id : rowOrder) {
            auto it = dataMap.find(id);
            if (it != dataMap.end()) {
                if (colIndex == 0)
                    cout << setw(columnWidth) << left << it->first << "\n";
                else {
                    if ((colIndex - 1) < it->second.values.size())
                        cout << setw(columnWidth) << left << it->second.values[colIndex - 1] << "\n";
                }
            }
        }
    }
}

void move_up(){
    fs::path currentPath = fs::current_path();
    if(currentTable != ""){
        dataMap.clear();  // Clear in-memory data for the current table.
        rowOrder.clear();
        cout << "Cleared in-memory data for table " << currentTable << endl;
        currentTable = "";
        return;
    }
    if(currentPath.string() == fs_path) {
        cout << "Already at the top-level directory (" << currentPath.string() << "). Exiting program." << endl;
        exitProgram = true;
    } else {
        fs::current_path("../");
        currentPath = fs::current_path();
        cout << "Moved up. Current working directory: " << currentPath.string() << endl;
        if(currentPath.string() == fs_path){
            currentDatabase = "";
            currentTable = "";
            cout << "Now at top-level DBMS directory. Prompt is now: dbms> " << endl;
        } else {
            currentTable = "";
            cout << "Exited table context. Current database: " << currentDatabase << endl;
        }
    }
}

// Transaction mechanism for table (CSV file) operations.
bool inTransaction = false;

// Reads metadata from "table_metadata.txt" in the current directory.
// Returns a mapping from table name to row count.
map<string, int> readTableMetadata() {
    map<string, int> metadata;
    string metaFileName = "table_metadata.txt";
    ifstream metaIn(metaFileName);
    if (!metaIn.is_open())
        return metadata; // Metadata file might not exist yet.
    string line;
    while(getline(metaIn, line)) {
        if(line.empty())
            continue;
        istringstream iss(line);
        string tableName;
        int rowCount;
        if (iss >> tableName >> rowCount)
            metadata[tableName] = rowCount;
    }
    metaIn.close();
    return metadata;
}

// Updates the metadata file with the row count for a given table.
// Reads the existing metadata, updates (or adds) the entry, then writes it back.
void updateTableMetadata(const string &tableName, int rowCount) {
    string metaFileName = "table_metadata.txt";
    map<string, int> metadata = readTableMetadata();
    metadata[tableName] = rowCount;
    ofstream metaOut(metaFileName);
    for (const auto &p : metadata) {
        metaOut << p.first << " " << p.second << "\n";
    }
    metaOut.close();
}


void beginTransaction(const string &tableName) {
    if(!inTransaction) {
        string filename = tableName + ".csv";
        string backupFilename = filename + ".bak";
        ifstream src(filename, ios::binary);
        ofstream dst(backupFilename, ios::binary);
        if(src && dst) {
            dst << src.rdbuf();
            inTransaction = true;
        }
    }
}

void commitTransaction(const string &tableName)
{
    string filename = tableName + ".csv";
    ofstream file(filename);
    if (!file.is_open())
    {
        cout << "Error opening file for commit." << endl;
        return;
    }
    // Write header row.
    for (size_t i = 0; i < headers.size(); i++)
    {
        file << headers[i];
        if (i < headers.size() - 1)
            file << ",";
    }
    file << "\n";
    // Write each row stored in dataMap in the order defined by rowOrder.
    // (Assumes a global vector<string> rowOrder is maintained.)
    for (const auto &id : rowOrder)
    {
        auto it = dataMap.find(id);
        if (it != dataMap.end()) {
            file << it->first;  // The ID
            for (const auto &val : it->second.values)
            {
                file << "," << val;
            }
            file << "\n";
        }
    }
    file.close();
    cout << "Commit successful. Changes written to file: " << fs::absolute(filename).string() << endl;
    // Update metadata file with current row count.
    updateTableMetadata(tableName, rowOrder.size());
}

// Rollback: Reload the in‑memory data from the CSV file.
void rollbackTransaction(const string &tableName)
{
    string filename = tableName + ".csv";
    retrieveData(filename);
    cout << "Rollback successful. In-memory data restored from file." << endl;
}


void make_table(list<string> &queryList) {
    if (queryList.empty()) {
        cout << "MAKE: Table name expected." << endl;
        return;
    }
    string tableName = queryList.front();
    queryList.pop_front();

    vector<string> headers;
    bool hasCustomHeaders = !queryList.empty();

    if (hasCustomHeaders) {
        string headersToken = "";
        while (!queryList.empty()) {
            headersToken += queryList.front();
            queryList.pop_front();
            if (!queryList.empty())
                headersToken += " ";
        }
        stringstream ss(headersToken);
        string token;
        while (getline(ss, token, ',')) {
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            if (!token.empty() && token.front() == '"' && token.back() == '"') {
                token = token.substr(1, token.size() - 2);
            }
            headers.push_back(token);
        }
    } else {
        headers = {"ID", "Name", "Age", "Department"};
    }

    if (currentDatabase.empty()) {
        cout << "No database selected. Use ENTER <database> to select a database." << endl;
        return;
    }

    string filename = tableName + ".csv";
    ifstream file(filename);
    if (file.good()) {
        cout << "Table already exists: " << fs::absolute(filename).string() << endl;
        return;
    }

    ofstream newTable(filename);
    if (newTable.is_open()) {
        for (size_t i = 0; i < headers.size(); i++) {
            newTable << headers[i];
            if (i < headers.size() - 1)
                newTable << ",";
        }
        newTable << "\n";
        newTable.close();

        cout << "Table created with headers: ";
        for (size_t i = 0; i < headers.size(); i++) {
            cout << headers[i];
            if (i < headers.size() - 1)
                cout << ", ";
        }
        cout << "\nLocation: " << fs::absolute(filename).string() << endl;

        currentTable = tableName;
        retrieveData(filename);
    } else {
        cerr << "Failed to create table file!" << endl;
    }
}


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

// Helper function to list all databases at root level.
void listDatabases() {
    fs::path rootPath = fs_path;  // Root DBMS folder
    if (!fs::exists(rootPath)) {
        cout << "Root DBMS directory not found." << endl;
        return;
    }
    for (const auto &entry : fs::directory_iterator(rootPath)) {
        if (entry.is_directory()) {
            string dbName = entry.path().filename().string();
            // Count CSV files (tables) in this database directory.
            int tableCount = 0;
            for (const auto &f : fs::directory_iterator(entry.path())) {
                // Only count CSV files, ignoring metadata file.
                if (f.is_regular_file() && f.path().extension() == ".csv" &&
                    f.path().stem().string() != "table_metadata")
                    tableCount++;
            }
            cout << dbName << " - " << tableCount << " tb" << endl;
        }
    }
}


// Lists tables in the current database using the metadata file.
void listTables() {
    fs::path dbPath = fs::current_path();  // Current directory is the selected database.
    map<string, int> metadata = readTableMetadata();
    for (const auto &entry : fs::directory_iterator(dbPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            string tableName = entry.path().stem().string();
            // Skip the metadata file.
            if (tableName == "table_metadata")
                continue;
            int rowCount = 0;
            // Use metadata if available.
            if (metadata.find(tableName) != metadata.end())
                rowCount = metadata[tableName];
            else {
                // Fallback: count rows in the file (skip header).
                ifstream file(entry.path());
                string line;
                bool header = true;
                while(getline(file, line)) {
                    if (header) { header = false; continue; }
                    if (!line.empty())
                        rowCount++;
                }
                file.close();
            }
            cout << tableName << " - " << rowCount << " rows" << endl;
        }
    }
}
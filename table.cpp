#include "row.cpp"
#include "library.cpp"  // Or your other necessary headers

struct Condition {
    string column;
    string op;     // Operator (e.g., =, >, <, <=, >=, / for not equal)
    string value;
};
extern string currentTable;
extern string fs_path;
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t"); // Finds the first character that is not a space or tab
    if(start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t"); // Finds the last such character that is not a space or tab
    return s.substr(start, end - start + 1); // ✅ The result includes the character at start_index,❌ But does NOT use an end_index.
}
// Helper for comparing two values based on an operator.
// bool compareValues(const string &actual, const string &op, const string &expected);
vector<string> extractValues(const string &command)
{
    vector<string> values;
    stringstream ss(command);
    string item;
    while (getline(ss, item, ',')) {
        // Trim spaces and remove quotes.
        item.erase(remove(item.begin(), item.end(), '"'), item.end());
        item.erase(remove(item.begin(), item.end(), '\''), item.end());
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        if(item.empty()){
            values.push_back("null");
        } else {
            values.push_back(item);
        }
    }
    if(item.empty()){
        values.push_back("null");
    }
    return values;
}
bool validateValue(const string &value, const string &dataType) {
    if(value == "null"){
        return true;
    }
    if (dataType == "INT") {
        try {
            size_t idx;
            stoi(value, &idx);
            if (idx != value.size())
                return false;
        } catch (...) {
            return false;
        }
        return true;
    }
    else if (dataType == "BIGINT") {
        try {
            size_t idx;
            stoll(value, &idx);
            if (idx != value.size())
                return false;
        } catch (...) {
            return false;
        }
        return true;
    }
    else if (dataType == "DOUBLE") {
        try {
            size_t idx;
            stod(value, &idx);
            if (idx != value.size())
                return false;
        } catch (...) {
            return false;
        }
        return true;
    }
    else if (dataType == "BIGDOUBLE") {
        try {
            size_t idx;
            stold(value, &idx);
            if (idx != value.size())
                return false;
        } catch (...) {
            return false;
        }
        return true;
    }
    else if (dataType == "CHAR") {
        // For CHAR, ensure the string length is exactly one.
        return value.size() == 1;
    }
    else if (dataType == "VARCHAR" || dataType == "STRING") {
        return !value.empty();
    }
    else if (dataType == "DATE") {
        // Validate date in format YYYY-MM-DD using regex.
        regex dateRegex("^\\d{4}-\\d{2}-\\d{2}$");
        if (!regex_match(value, dateRegex))
            return false;
        // Further validate the date components.
        int year, month, day;
        try {
            year = stoi(value.substr(0, 4));
            month = stoi(value.substr(5, 2));
            day = stoi(value.substr(8, 2));
        } catch (...) {
            return false;
        }
        if (month < 1 || month > 12)
            return false;
        // Check days in month (basic check, does not fully account for leap years)
        int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (day < 1 || day > daysInMonth[month - 1])
            return false;
        return true;
    }
    else if (dataType == "BOOL") {
        string lowerValue = value;
        transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
        return (lowerValue == "true" || lowerValue == "false" || lowerValue == "1" || lowerValue == "0");
    }
    // Additional data types can be added here.
    return false;
}
bool compareValues(const string &actual, const string &op, const string &expected) {
    // Try to treat both values as numbers.
    double a, b;
    bool isNumeric = false;
    try {
        a = stod(actual);
        b = stod(expected);
        isNumeric = true;
    } catch (...) {
        // Not numeric; fall back to string comparison.
    }
    if (op == "=")
        return actual == expected;
    else if (op == ">" )
        return isNumeric ? (a > b) : (actual > expected);
    else if (op == "<")
        return isNumeric ? (a < b) : (actual < expected);
    else if (op == ">=")
        return isNumeric ? (a >= b) : (actual >= expected);
    else if (op == "<=")
        return isNumeric ? (a <= b) : (actual <= expected);
    else if (op == "!=")
        return actual != expected;
    return false;
}
// class declaration
class Table {
private:
    // Table-specific members.
    string tableName;
    string filename;
    vector<string> headers;
    unordered_map<string, Row> dataMap;
    vector<string> rowOrder;  // Keeps row IDs in CSV insertion order.
    unordered_map<string, pair<string, string>> columnMeta;
    int primaryKeyIndex; 
    int columnWidth;
    bool unsavedChanges;

    void writeToFile() {
        ofstream file(filename);
        if (!file.is_open())
        {
            throw ("program_error: could not commit changes.");
        }
        
        // Write header row.
        for (size_t i = 0; i < headers.size(); i++) {
            string colName = headers[i];
            auto meta = columnMeta[colName];
            string headerLine = colName + "(" + meta.first + ")";
            if (!meta.second.empty()) {
                istringstream ss(meta.second);
                string constraint;
                while (getline(ss, constraint, ',')) {
                    constraint = trim(constraint); // optional: remove spaces
                    if (!constraint.empty())
                        headerLine += "(" + constraint + ")";
                }
            }
            file << headerLine;
            if (i < headers.size() - 1)
                file << ",";
        }
        file << "\n";
        
        // Write each row in the order defined by rowOrder.
        // Reconstruct the row based on primaryKeyIndex.
        for (const auto &id : rowOrder)
        {
            auto it = dataMap.find(id);
            if (it != dataMap.end()) {
                // Reassemble the row in the order specified by headers.
                // For each column index, if it's the primary key column, use it->first (id);
                // otherwise, use the corresponding value from it->second.values.
                for (size_t i = 0; i < headers.size(); i++) {
                    string cell;
                    if ((int)i == primaryKeyIndex) {
                        cell = it->first; // primary key value.
                    } else if ((int)i < primaryKeyIndex) {
                        cell = (i < it->second.values.size()) ? it->second.values[i] : "";
                    } else { // i > primaryKeyIndex: adjust index by subtracting one.
                        int valIndex = i - 1;
                        cell = (valIndex < it->second.values.size()) ? it->second.values[valIndex] : "";
                    }
                    file << cell;
                    if (i < headers.size() - 1)
                        file << ",";
                }
                file << "\n";
            }
        }
        file.close();
    }
              
    // Reads metadata from "table_metadata.txt" in the current directory.
    map<string, int> readTableMetadata(const string &metaFileName = "table_metadata.txt") {
        map<string, int> metadata;
        ifstream metaIn(metaFileName);
        if (!metaIn.is_open()) {
            // Metadata file might not exist yet; return an empty map.
            return metadata;
        }
        string line;
        while (getline(metaIn, line)) {
            if (line.empty())
                continue;
            
            istringstream iss(line);
            string tableName, dash, rowsWord;
            int rowCount;
            // Expecting the format: tableName - rowCount rows
            if (iss >> tableName >> dash >> rowCount >> rowsWord) {
                if (dash == "-" && rowsWord == "rows") {
                    metadata[tableName] = rowCount;
                }
            }
        }
        metaIn.close();
        return metadata;
    }
    
    // Updates the metadata file with the row count for a given table.
    // The metadata file is updated with each entry in the following format:
    // tableName - rowCount rows
    void updateTableMetadata(const string &metaFileName = "table_metadata.txt") {
        // Read current metadata.
        string metaFilePath = (fs::path(fs_path) / (metaFileName + ".csv")).string();
        map<string, int> metadata = readTableMetadata(metaFilePath);
        // Update the metadata for the provided table.
        metadata[tableName] = static_cast<int>(rowOrder.size());
        
        // Write the updated metadata back to the file.
        ofstream metaOut(metaFilePath);
        if (!metaOut.is_open()) {
            return ;
        }
        for (const auto &entry : metadata) {
            // Write in the format: tableName - rowCount rows
            metaOut << entry.first << " - " << entry.second << " rows" << "\n";
        }
        metaOut.close();
    }

public:
    // Constructor: given a table name, it sets filename and retrieves data.
    /*           DONE            */
    Table(const string &tName) 
      : tableName(tName), filename(tName + ".csv"), columnWidth(15) ,unsavedChanges(false)
    {
        retrieveData();
    }

    // Destructor: clear in-memory data to prevent leaks.
    ~Table() {
        dataMap.clear();
        rowOrder.clear();
        headers.clear();
        columnMeta.clear();
    }
    vector<vector<Condition>> parseAdvancedConditions(const vector<string>& tokens);
    // For checking if a row or column exists.
    bool hasRow(const string &id);
    bool hasColumn(const string &colName);
    void setPrimaryKeyIndex(int index) { primaryKeyIndex = index; }
    // For deleting an entire column.
    void deleteColumn(const string &colName);
    // For deleting rows based on advanced conditions.
    void deleteRowsByAdvancedConditions(const vector<vector<Condition>> &groups);
    bool evaluateAdvancedConditions(const Row &row, const vector<vector<Condition>> &groups);
    // Overload for updating a specified column.
    void updateValueByCondition(const string &colName, const string &oldValue, const string &newValue,
        const vector<vector<Condition>> &conditionGroups);

    // Overload for updating across all columns (except the primary key).
    void updateValueByCondition(const string &oldValue, const string &newValue,
        const vector<vector<Condition>> &conditionGroups);

    /* DONE */
    // Retrieve data from CSV file into in-memory structures.
    void retrieveData() {
        dataMap.clear();
        rowOrder.clear();
        headers.clear();
        columnMeta.clear();
        
        // Reset primaryKeyIndex to an invalid value.
        primaryKeyIndex = -1;
        
        ifstream file(filename);
        if (!file.is_open()) {
            throw ("program_error: Error occured while opening " + filename + ". Try later. If persists check docs");
        }
        string line;
        bool isHeader = true;
        while (getline(file, line)) {
            stringstream ss(line); // stringstream is part of the C++ Standard Library in <sstream>. It works like a stream (just like cin or cout),
            // but instead of reading/writing to the console or files, it reads/writes to a string in memory.
            vector<string> rowValues;
            string value;
            while (getline(ss, value, ',')) { // here , is delimitters
                rowValues.push_back(value);
            }
            if(value.empty()){
                rowValues.push_back(""); //////////////////////// -------------- This is were i can add null while retriving ---------------------- ////////////////////////////////////
            }
            if (isHeader) {
                // Parse header row to fill headers and columnMeta.
                // Also detect which column is designated as the PRIMARY_KEY.
                int colIndex = 0;
                for (auto &col : rowValues) {
                    size_t firstParen = col.find('(');
                    size_t firstClose = col.find(')', firstParen);
                    if (firstParen != string::npos && firstClose != string::npos) {
                        // Extract column name and data type.
                        string colName = trim(col.substr(0, firstParen));
                        string dataType = trim(col.substr(firstParen + 1, firstClose - firstParen - 1));
                        // Collect multiple constraints.
                        vector<string> constraints;
                        size_t currentPos = firstClose + 1;
                        while (true) {
                            size_t open = col.find('(', currentPos);
                            size_t close = col.find(')', open);
                            if (open == string::npos || close == string::npos)
                                break;
                            string constraint = trim(col.substr(open + 1, close - open - 1));
                            constraints.push_back(constraint);
                            currentPos = close + 1;
                        }
                        string allConstraints; // ------------------------------------- can be converted here
                        for (size_t i = 0; i < constraints.size(); ++i) {
                            allConstraints += constraints[i];
                            if (i != constraints.size() - 1)
                                allConstraints += ","; // Join constraints with comma.
                        }
                        headers.push_back(colName);
                        columnMeta[colName] = {dataType, allConstraints};
                        
                        // Check if this column is the PRIMARY_KEY.
                        for (auto &c : constraints) {
                            if (c == "PRIMARY") {
                                // Set the primary key index.
                                primaryKeyIndex = colIndex;
                                // cout<<primaryKeyIndex;
                                break;
                            }
                        }
                        colIndex++;
                    }
                }
                isHeader = false;
                // If no primary key was detected, you might want to print an error.
                if (primaryKeyIndex == -1) {
                    // cout << "Warning: No PRIMARY_KEY detected in header." << endl;
                    // thiking of implementing to add a primary column ourselves
                    // Depending on your design, you could choose to exit here.
                }
            } else {
                if (!rowValues.empty()) {
                    // Ensure that the row has as many values as there are headers.
                    if (rowValues.size() < headers.size() || rowValues.size() > headers.size()) {
                        // cout << "Row skipped: insufficient/more number of columns." << endl;
                        continue;
                    }
                    // Extract the primary key value using primaryKeyIndex.
                    string pkValue = rowValues[primaryKeyIndex];
                    // Build a new vector of values that excludes the primary key column.
                    vector<string> otherValues;
                    for (size_t i = 0; i < rowValues.size(); i++) {
                        if (i == (size_t)primaryKeyIndex)
                            continue;
                        otherValues.push_back(rowValues[i]);
                    }
                    // Insert into dataMap using the primary key value.
                    dataMap[pkValue] = Row(pkValue, otherValues);
                    rowOrder.push_back(pkValue);
                }
            }
        }
        file.close();
    }
    #include <sstream>  // For istringstream

    unordered_set<string> parseConstraints(const string &constraintStr) {
        unordered_set<string> constraints;
        istringstream ss(constraintStr);
        string token;
        
        while (getline(ss, token, ',')) {
            token = trim(token);  // Optional: remove leading/trailing spaces
            if (!token.empty()) {
                constraints.insert(token);
            }
        }
    
        return constraints;
    }
    void insertRow(const string &command) {
        vector<string> values = extractValues(command);
        // bool allNull = true;
        // for(int i = 0; i < values.size();i++){
        //     if(values[i] != "null"){
        //         allNull = !allNull;
        //         break;
        //     }
        // }
        // if (values.empty() || allNull) {
        //     throw invalid_argument("INSERT -> missing values/delimitters(,).");
        // }
        int count = values.size();
        int n_cols = headers.size();
        if (count > n_cols || count < n_cols) {
            string errMsg = "invalid_argument: Number of values (" + to_string(count) + ") does not match number of columns (" + to_string(n_cols) + ").";
            throw (errMsg);
        }
        for (size_t i = 0; i < values.size(); i++) {
            string colName = headers[i];
            string expectedType = columnMeta[colName].first;
            string consStr = columnMeta[colName].second;
            unordered_set<string> consSet = parseConstraints(consStr);
        
            
            // #include <algorithm>  // for std::find_if

            // check for default
            auto it = std::find_if(consSet.begin(), consSet.end(), [](const std::string &s) {
                return s.compare(0, 7, "DEFAULT") == 0;
            });
            if (it != consSet.end()) {
                // Constraint found. Extract the substring after '#'
                std::string constraint = *it;
                std::size_t pos = constraint.find('#');
                if (pos != std::string::npos && pos + 1 < constraint.size()) {
                    std::string defaultValue = constraint.substr(pos + 1);
                    if(!validateValue(defaultValue, expectedType)){
                        throw ("mismatch_error: " + defaultValue + " doesn't match " + colName + " datatype.");
                    }
                    // Check for "null" or empty trimmed value before applying the default.
                    if (values[i] == "null" || trim(values[i]).empty()) {
                        values[i] = defaultValue;
                    }
                }
            }
            // Check NOT_NULL constraint.
            if (consSet.count("NOT_NULL")) {
                if (values[i] == "null" || trim(values[i]).empty()) {
                    throw ("Constraint Error: Column '" + colName + "' cannot be null.");
                }
            }
        
            // Check UNIQUE constraint.
            if (consSet.count("UNIQUE")) {
                // For the primary key, dataMap keys already hold the value.
                // For other columns, iterate over all rows.
                for (const auto &pair : dataMap) {
                    string existingVal;
                    if ((int)i == primaryKeyIndex)
                        existingVal = pair.first;
                    else {
                        int idx = (i < (size_t)primaryKeyIndex) ? i : i - 1;
                        existingVal = (idx < pair.second.values.size()) ? pair.second.values[idx] : "";
                    }
                    if (existingVal == values[i]) {
                        throw ("Constraint Error: Duplicate value '" + values[i] +
                                            "' found in UNIQUE column '" + colName + "'.");
                    }
                }
            }
        
            // AUTO_INCREMENT is handled for primary key (and optionally other columns) as in section 3.
            if (consSet.count("AUTO_INCREMENT") || consSet.count("PRIMARY")) {
                if (values[i] == "null" || trim(values[i]).empty()) {
                    int maxVal = 0;
                    // Iterate through all rows to find the current maximum value.
                    for (const auto &pair : dataMap) {
                        string curVal;
                        if ((int)i == primaryKeyIndex)
                            curVal = pair.first;
                        else {
                            int idx = (i < (size_t)primaryKeyIndex) ? i : i - 1;
                            curVal = (idx < pair.second.values.size()) ? pair.second.values[idx] : "0";
                        }
                        try {
                            int num = stoi(curVal);
                            maxVal = max(maxVal, num);
                        } catch (...) {
                            // Non-numeric values are ignored.
                        }
                    }
                    values[i] = to_string(maxVal + 1);
                }
            }
            // Also check that the value conforms to the data type.
            if (!validateValue(values[i], expectedType)) {
                throw ("Mismatch Error: Value \"" + values[i] + "\" is not valid for column \"" 
                                       + colName + "\" of type " + expectedType + ".");
            }
        }
        // Check primary key constraint
        string pkValue = values[primaryKeyIndex];
        if (dataMap.find(pkValue) != dataMap.end()) {
            throw ("Constraint Error: Primary Key " + pkValue + " already exists.");
            return;
        } 
        else {
            if (pkValue == "null" || trim(pkValue).empty()) {
                // Auto-increment: iterate over dataMap to find the maximum numeric primary key.
                int maxVal = 0;
                for (const auto &pair : dataMap) {
                    try {
                        int num = stoi(pair.first);
                        if (num > maxVal) {
                            maxVal = num;
                        }
                    } catch (...) {
                        // If conversion fails, ignore (or log error).
                    }
                }
                // Next auto-increment value.
                pkValue = to_string(maxVal + 1);
                values[primaryKeyIndex] = pkValue;
            }
        }
    
        // Prepare values excluding primary key
        vector<string> rowValues;
        for (size_t i = 0; i < values.size(); i++) {
            if (i == (size_t)primaryKeyIndex)
                continue;
            rowValues.push_back(values[i]);
        }
    
        dataMap[pkValue] = Row(pkValue, rowValues);
        rowOrder.push_back(pkValue);
        unsavedChanges = true;
    }
    
    void deleteRow(const string &id) {
        auto it = dataMap.find(id);
        if (it != dataMap.end()) {
            dataMap.erase(it);
            rowOrder.erase(remove(rowOrder.begin(), rowOrder.end(), id), rowOrder.end());
            // else: silent deletion or custom logic
        }
        unsavedChanges = true;
    }
    
    
    // Clear all rows from the table (keeping headers intact).
    void cleanTable() {
        dataMap.clear();
        rowOrder.clear();
        unsavedChanges = true;
    }   
    void commitTransaction() {
        writeToFile();
        updateTableMetadata();
        unsavedChanges = false;
        cout << "\033[32mres: Commit successful.\033[0m" << endl;
    }
    void rollbackTransaction() {
        if(unsavedChanges){
            retrieveData();
        }else{
            cerr << "WARNING: No changes made to table." << endl;
        }
        cout << "\033[32mres: Rollback successful.\033[0m" << endl;
    }
    void describe() {
        // Print a header for the description.
        cout << "Table: " << currentTable << "\n";
        cout << "-------------------------------------------------\n";
        cout <<  "\033[33m" << setw(20) << left << "Column Name" 
             << setw(15) << left << "Data Type"
             << "Constraints" << "\033[0m\n";
            /* 
                left and setw are I/O manipulators in C++ from the <iomanip> header, 
                used to format how text is printed to the console 
                (especially when aligning columns nicely, like in tables).
            */
        cout << "-------------------------------------------------\n";
    
        // Iterate over the headers vector.
        for (const auto &colName : headers) {
            // Get data type and constraints from columnMeta.
            auto metaIt = columnMeta.find(colName);
            string dataType = (metaIt != columnMeta.end()) ? metaIt->second.first : "\033[31m*None\033[0m";
            string constraints = (metaIt != columnMeta.end()) ? metaIt->second.second : "\033[31m*None\033[0m";
            if(trim(dataType).empty()) dataType = "\033[31mNone\033[0m";
            if(trim(constraints).empty()) constraints = "\033[31mNone\033[0m";
            cout << setw(20) << left << colName
                 << setw(15) << left << dataType
                 << constraints << "\n";
        }
        cout << "-------------------------------------------------\n";
    }
    
    void show(const string &params) {
        // Tokenize parameters.
        istringstream iss(params);
        vector<string> tokens;
        string token;
        while (iss >> token)
            tokens.push_back(token);
        if (tokens.empty()) {
            throw "syntax_error: SHOW -> missing arguments.\n";
        }
        
        // --- Check for LIKE clause ---
        bool likeMode = false;
        string likePattern;  // prefix pattern for LIKE search (without trailing '*')
        // Look for the token "LIKE" (case-insensitive)
        for (size_t i = 0; i < tokens.size(); i++) {
            string t = tokens[i];
            // string upperT = t;
            // transform(upperT.begin(), upperT.end(), upperT.begin(), ::toupper);
            if (t == LIKE) {
                likeMode = true;
                if (i + 1 < tokens.size()) {
                    likePattern = trim(tokens[i + 1]);
                    if (!likePattern.empty() && ((likePattern.front() == '"' && likePattern.back() == '"') ||
                                                 (likePattern.front() == '\'' && likePattern.back() == '\'')))
                        likePattern = likePattern.substr(1, likePattern.size() - 2);
                    if (!likePattern.empty() && likePattern.back() == '*')
                        likePattern.pop_back();
                } else {
                    throw ("syntax_error: SHOW -> missing argument for LIKE clause.");
                }
                // Erase the LIKE token and its argument.
                tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
                break;
            }
        }
        
        // --- Helper Lambdas in Outer Scope ---
        
        // Total number of columns.
        size_t nCols = headers.size();
    
        // Center a string within a given width.
        auto center = [&](const string &s, size_t width) -> string {
            if (s.length() >= width)
                return s;
            size_t leftPadding = (width - s.length()) / 2;
            size_t rightPadding = width - s.length() - leftPadding;
            return string(leftPadding, ' ') + s + string(rightPadding, ' ');
        };
    
        // Compute initial column widths based on header names.
        vector<size_t> colWidths(nCols, 0);
        for (size_t i = 0; i < nCols; i++) {
            colWidths[i] = headers[i].length();
        }
    
        // Given a primary key, fetch the entire row as a vector of cell strings.
        auto getRowCells = [&](const string &pk) -> vector<string> {
            vector<string> cells(nCols, "");
            auto it = dataMap.find(pk);
            if (it == dataMap.end())
                return cells;
            for (size_t i = 0; i < nCols; i++) {
                string cell;
                if ((int)i == primaryKeyIndex)
                    cell = it->first;
                else if ((int)i < primaryKeyIndex)
                    cell = (i < it->second.values.size()) ? it->second.values[i] : "";
                else {
                    int valIndex = i - 1;
                    cell = (valIndex < it->second.values.size()) ? it->second.values[valIndex] : "";
                }
                cells[i] = cell;
            }
            return cells;
        };
        // this is for all columns
        auto print = [&](const bool& printRows, const bool &dir,const int& nor) -> void {
            int count = 0;
            int total = rowOrder.size();
            if(nor > total){
                string errMsg = to_string(total) + "records are present.";
                throw invalid_argument(errMsg);
            }
            // Update column widths based on row content.
            if(dir){ // if true top -> bottom
                for (const auto &pk : rowOrder) {
                    if(count < nor) count ++; // inno setup compiler
                    else break;
                    vector<string> cells = getRowCells(pk);
                    for (size_t i = 0; i < nCols; i++) {
                        colWidths[i] = max(colWidths[i], cells[i].length());
                    }
                }
            } else { // if false bottom -> top
                count = total - nor;
                for(int i = count ; i < total; i++){
                    vector<string> cells = getRowCells(rowOrder[i]);
                    for (size_t j = 0; j < nCols; j++) {
                        colWidths[j] = max(colWidths[j], cells[j].length());
                    }
                }
            }
            
            // above columns defs
            cout << endl;
            for (size_t i = 0; i < nCols; i++) {
                if(i == 0) cout << "+-";
                else if (i != nCols) cout << "-+-";
                cout << string(colWidths[i], '-');
                
            }
            // column definations 
            cout << "-+\n";
            for (size_t i = 0; i < nCols; i++) {
                if(i == 0) cout << "| ";
                else if (i != nCols) cout << " | ";
                cout << "\033[33m" << center(headers[i], colWidths[i]) << "\033[0m";
            }
            // below column defs
            cout << " |\n";
            for (size_t i = 0; i < nCols; i++) {
                if(i == 0) cout << "+-";
                else if (i != nCols) cout << "-+-";
                cout << string(colWidths[i], '-');
                
            }
            cout << "-+\n";
            if(printRows){
                // --- Inline printing each row.
                if (dir) {
                    for (const auto &pk : rowOrder) {
                        if (count < 1) break;
                        vector<string> cells = getRowCells(pk);
                        for (size_t i = 0; i < nCols; i++) {
                            if(i == 0) cout << "| ";
                            else if (i != nCols) cout << " | ";
                            cout << setw(colWidths[i]) << left << cells[i];
                        }
                        cout << " |\n";
                        count--;
                    }
                } else {
                    int start = max(0, total - nor);
                    for (int i = start; i < total ; i++) {
                        vector<string> cells = getRowCells(rowOrder[i]);
                        for (size_t j = 0; j < nCols; j++) {
                            if(j == 0) cout << "| ";
                            else if (j != nCols) cout << " | ";
                            cout << setw(colWidths[j]) << left << cells[j];
                        }
                        cout << " |\n";
                    }
                }
                // table below column defs
                for (size_t i = 0; i < nCols; i++) {
                    if(i == 0) cout << "+-";
                    else if (i != nCols) cout << "-+-";
                    cout << string(colWidths[i], '-');
                }
                cout << "-+\n";
            }
        };
    
        // ----- START: Mode branches that use in-line printing logic -----
        
        // Mode 1: SHOW * [WHERE/LIKE clauses]
        if (tokens[0] == "*") {
            tokens.erase(tokens.begin()); // remove "*"
            // Check for an optional WHERE clause.
            bool whereClause = false;
            vector<string> condTokens;
            if (!tokens.empty()) {
                string firstToken = tokens[0];
                if (firstToken == WHERE) {
                    whereClause = true;
                    tokens.erase(tokens.begin()); // Remove "WHERE"
                    condTokens = tokens;  // Remaining tokens form the WHERE clause.
                    if (condTokens.empty())
                        throw ("syntax_error: SHOW -> WHERE clause provided but missing conditions.");
                }
            }
            
            // Lambda for filtering by LIKE (applies to all string columns).
            auto rowMatchesLike = [&](const vector<string>& cells) -> bool {
                for (size_t i = 0; i < nCols; i++) {
                    string dt = columnMeta[headers[i]].first;
                    if (dt == "VARCHAR" || dt == "CHAR" || dt == "STRING") {
                        string cell = cells[i];
                        if (cell.length() >= likePattern.length() &&
                            cell.substr(0, likePattern.length()) == likePattern)
                            return true;
                    }
                }
                return false;
            };
            // prints the table
            print(false,true,rowOrder.size());
            auto condGroups = parseAdvancedConditions(condTokens);
            for (const auto &pk : rowOrder) {
                auto it = dataMap.find(pk);
                vector<string> cells = getRowCells(pk);
                if (likeMode && !rowMatchesLike(cells))
                    continue;
                if (!condTokens.empty() && !evaluateAdvancedConditions(it->second, condGroups))
                    continue;
                for (size_t i = 0; i < nCols; i++) {
                    if(i == 0)
                        cout << "| ";
                    else if (i != nCols)
                        cout << " | ";
                    cout << setw(colWidths[i]) << left << cells[i];
                }
                cout << " |\n";
            }
            for (size_t i = 0; i < nCols; i++) {
                if(i == 0)
                    cout << "+-";
                else if (i != nCols)
                    cout << "-+-";
                cout << string(colWidths[i], '-');
                
            }
            cout << "-+\n";
        }
        // Mode 2: HEAD - Display header and first 5 rows (using the same in-line printing logic)
        else if (tokens[0] == HEAD) {
            int defaultLimit = 5;
            print(true,true,defaultLimit);
        }
        // Mode 3: LIMIT - Display header and a limited number of rows.
        else if (tokens[0] == LIMIT) {
            if (tokens.size() < 2) {
                throw ("syntax_error: LIMIT -> missing number for LIMIT command.");
            }
            bool fromBottom = false;
            string numToken = tokens[1];
            if (!numToken.empty() && numToken[0] == TILDE) {
                fromBottom = true;
                numToken = numToken.substr(1);
            }
            int limit = 0;
            try {
                limit = stoi(numToken);
            } catch (...) {
                throw ("syntax_error: LIMIT -> invalid number for LIMIT command.");
            }
            if (limit <= 0) {
                throw ("syntax_error: LIMIT -> limit must be a positive integer.");
            }
            print(true,!fromBottom,limit);
        }
        // Mode 4: SHOW specific columns with optional WHERE/LIKE filtering.
        else {
            // This branch handles "show <col1>,<col2>,... [where/LIKE <conditions>]"
            // Instead of joining tokens, treat each token before a WHERE/LIKE as a separate column spec.
            vector<string> selectedColumns;
            int clauseIndex = -1;
            for (int i = 0; i < tokens.size(); i++) {
                string token = tokens[i];
                if (token == WHERE) {
                    clauseIndex = i;
                    break;
                }
                // Trim and remove surrounding quotes.
                string colName = trim(tokens[i]);
                if (!colName.empty() && ((colName.front() == '"' && colName.back() == '"') ||
                                         (colName.front() == '\'' && colName.back() == '\'')))
                    colName = colName.substr(1, colName.size() - 2);
                if (!colName.empty())
                    selectedColumns.push_back(colName);
            }
    
            if (selectedColumns.empty()) {
                throw ("syntax_error: SHOW -> no columns specified.");
            }
            
            // Determine if there's an extra clause.
            vector<string> extraTokens;
            if (clauseIndex != -1) {
                extraTokens.assign(tokens.begin() + clauseIndex + 1, tokens.end());
            }
            
            // If the clause is LIKE, then set likeMode.
            if (clauseIndex != -1) {
                string firstClause = tokens[clauseIndex];
                if (firstClause == LIKE) {
                    likeMode = true;
                    if (!extraTokens.empty()) {
                        likePattern = trim(extraTokens[0]);
                        if (!likePattern.empty() &&
                            ((likePattern.front() == '"' && likePattern.back() == '"') ||
                             (likePattern.front() == '\'' && likePattern.back() == '\'')))
                            likePattern = likePattern.substr(1, likePattern.size() - 2);
                        if (!likePattern.empty() && likePattern.back() == '*')
                            likePattern.pop_back();
                    } else {
                        throw ("syntax_error: SHOW -> missing argument for LIKE clause.");
                    }
                    extraTokens.clear(); // clear extraTokens since LIKE is handled separately.
                }
            }
            
            // Validate each selected column by finding its index in headers.
            vector<int> colIndices;
            for (auto &colName : selectedColumns) {
                int colIndex = -1;
                for (int i = 0; i < headers.size(); i++) {
                    if (headers[i] == colName) {
                        colIndex = i;
                        break;
                    }
                }
                if (colIndex == -1) {
                    throw ("Column \"" + colName + "\" not found.\n");
                }
                colIndices.push_back(colIndex);
            }
            
            // Compute dynamic widths for selected columns.
            size_t nSelected = colIndices.size();
            vector<size_t> selColWidths(nSelected, 0);
            for (size_t i = 0; i < nSelected; i++) {
                selColWidths[i] = headers[colIndices[i]].length();
            }
            
            auto getCellValue = [&](const Row &row, int colIndex) -> string {
                string cell;
                if (colIndex == primaryKeyIndex)
                    cell = row.id;
                else if (colIndex < primaryKeyIndex)
                    cell = (colIndex < row.values.size()) ? row.values[colIndex] : "";
                else {
                    int valIndex = colIndex - 1;
                    cell = (valIndex < row.values.size()) ? row.values[valIndex] : "";
                }
                return cell;
            };
            
            // Update widths based on row content.
            for (const auto &pk : rowOrder) {
                auto it = dataMap.find(pk);
                if (it == dataMap.end()) continue;
                for (size_t i = 0; i < nSelected; i++) {
                    string cell = getCellValue(it->second, colIndices[i]);
                    selColWidths[i] = max(selColWidths[i], cell.length());
                }
            }
            
            // Re-use the center lambda.
            auto centerSel = [&](const string &s, size_t width) -> string {
                if (s.length() >= width)
                    return s;
                size_t leftPadding = (width - s.length()) / 2;
                size_t rightPadding = width - s.length() - leftPadding;
                return string(leftPadding, ' ') + s + string(rightPadding, ' ');
            };
            
            // Print header row (centered) for selected columns.
            cout << endl;
            for (size_t i = 0; i < selectedColumns.size(); i++) {
                if (i == 0) cout << "+-";
                else if (i != selectedColumns.size()) cout << "-+-";
                cout << string(selColWidths[i], '-');
            }
            cout << "-+\n";
            for (size_t i = 0; i < selectedColumns.size(); i++) {
                if (i == 0) cout << "| ";
                else if(i != selectedColumns.size()) cout << " | ";
                cout << "\033[33m" << centerSel(selectedColumns[i], selColWidths[i]) << "\033[0m";
            }
            cout << " |\n";
            for (size_t i = 0; i < selectedColumns.size(); i++) {
                if (i == 0) cout << "+-";
                else if (i != selectedColumns.size()) cout << "-+-";
                cout << string(selColWidths[i], '-');
            }
            cout << "-+\n";
            
            // Parse WHERE conditions if extraTokens exist.
            vector<vector<Condition>> conditionGroups;
            if (!extraTokens.empty())
                conditionGroups = parseAdvancedConditions(extraTokens);
            
            // For LIKE filtering on selected columns.
            auto rowMatchesLikeSel = [&](const Row &row) -> bool {
                for (int colIndex : colIndices) {
                    string dt = columnMeta[headers[colIndex]].first;
                    if (dt == "VARCHAR" || dt == "CHAR" || dt == "STRING") {
                        string cell = getCellValue(row, colIndex);
                        if (cell.length() >= likePattern.length() &&
                            cell.substr(0, likePattern.length()) == likePattern)
                            return true;
                    }
                }
                return false;
            };
            
            // Print each row.
            for (const auto &pk : rowOrder) {
                auto it = dataMap.find(pk);
                if (it == dataMap.end())
                    continue;
                if (!conditionGroups.empty() && !evaluateAdvancedConditions(it->second, conditionGroups))
                    continue;
                if (likeMode && !rowMatchesLikeSel(it->second))
                    continue;
                for (size_t i = 0; i < nSelected; i++) {
                    string cell = getCellValue(it->second, colIndices[i]);
                    if(i == 0) cout << "| ";
                    else if (i != nCols) cout << " | ";
                    cout << setw(selColWidths[i]) << left << cell;
                }
                cout << " |\n";
            }
            for (size_t i = 0; i < selectedColumns.size(); i++) {
                if (i == 0) cout << "+-";
                else if (i != selectedColumns.size()) cout << "-+-";
                cout << string(selColWidths[i], '-');
            }
            cout << "-+\n";
        }
    }
    // End of show() function.
    friend void exitTable();
};
    
bool Table::hasRow(const string &id) {
    return dataMap.find(id) != dataMap.end();
}

bool Table::hasColumn(const string &colName) {
    for (const auto &header : headers) {
        if (header == colName)
            return true;
    }
    return false;
}
void Table::deleteColumn(const string &colName) {
    // Find the column index.
    int colIndex = -1;
    for (int i = 0; i < headers.size(); i++) {
        if (headers[i] == colName) {
            colIndex = i;
            break;
        }
    }
    if (colIndex == primaryKeyIndex) {  // Prevent deletion of primary key.
        throw invalid_argument("Primary key column cannot be deleted.");
    }
    // Remove from headers and metadata.
    headers.erase(headers.begin() + colIndex);
    columnMeta.erase(colName);
    // Remove the corresponding value from each row.
    for (auto &pair : dataMap) {
        if (pair.second.values.size() >= colIndex)
            pair.second.values.erase(pair.second.values.begin() + (colIndex - 1));
    }
    cout << "\033[32mres: Column \"" << colName << "\" deleted successfully.\033[0m" << endl;
    unsavedChanges = true;
}
void Table::deleteRowsByAdvancedConditions(const vector<vector<Condition>> &groups) {
    vector<string> rowsToDelete;

    for (const auto &id : rowOrder) {
        auto it = dataMap.find(id);
        if (it != dataMap.end() && evaluateAdvancedConditions(it->second, groups)) {
            rowsToDelete.push_back(id);
        }
    }
    // Delete the rows that satisfy the condition.
    for (const auto &id : rowsToDelete) {
        dataMap.erase(id);
        rowOrder.erase(remove(rowOrder.begin(), rowOrder.end(), id), rowOrder.end());
    }
    cout <<"\033[32mres: " << rowsToDelete.size() << " row(s) affected.\033[0m" << endl;
    unsavedChanges = true;
}
bool Table::evaluateAdvancedConditions(const Row &row, const vector<vector<Condition>> &groups) {
    // Each group is OR-connected; conditions within a group are AND-connected.
    for (const auto &group : groups) {
        bool groupSatisfied = true;
        for (const auto &cond : group) {
            // Locate the column index.
            int colIndex = -1;
            for (int i = 0; i < headers.size(); i++) {
                if (headers[i] == cond.column) {
                    colIndex = i;
                    break;
                }
            }
            if (colIndex == -1) { groupSatisfied = false; break; }
            string actual;
            if (colIndex == primaryKeyIndex) {
                actual = row.id;
            } 
            else if (colIndex < primaryKeyIndex) {
                // Before the primary key: use the same index.
                actual = (colIndex < row.values.size()) ? row.values[colIndex] : "";
            } 
            else {  // colIndex > primaryKeyIndex
                // After the primary key: the mapping is offset by one.
                int idx = colIndex - 1;
                actual = (idx < row.values.size()) ? row.values[idx] : "";
            }
            if (!compareValues(actual, cond.op, cond.value)) {
                groupSatisfied = false;
                break;
            }
        }
        if (groupSatisfied)
            return true;
    }
    return false;
}

vector<vector<Condition>> Table::parseAdvancedConditions(const vector<string>& tokens) {
    // so logically speaking for this cond1 AND cond2 OR cond3 AND cond4
    // this is how it gets stored in groups: 
    /*[
        [cond1, cond2],
        [cond3, cond4]
      ] 
    */
    vector<vector<Condition>> groups;
    vector<Condition> currentGroup;


    auto trimQuotes = [](const string &s) -> string {
        string trimmed = trim(s);
        if (!trimmed.empty() && ((trimmed.front() == '"' && trimmed.back() == '"') || (trimmed.front() == '\'' && trimmed.back() == '\'')))
            trimmed = trimmed.substr(1, trimmed.size() - 2);
        return trimmed;
    };

    int i = 0;
    while (i < tokens.size()) {
        if (i + 2 < tokens.size()) {
            Condition cond;
            cond.column = trimQuotes(tokens[i]);
            cond.op = tokens[i + 1];
            cond.value = trimQuotes(tokens[i + 2]);

            // --- Column name validation here ---
            bool columnExists = false;
            for (const auto &header : headers) {
                if (header == cond.column) {
                    string dataType = columnMeta[cond.column].first;
                    if( cond.value == "null" || !validateValue(cond.value,dataType)){
                        throw ("mismatch_error: Value "+ cond.value +" is not valid for column " + cond.column + " of type " + dataType + ".");
                    }
                    columnExists = true;
                    break;
                }
            }
            if (!columnExists) {
                throw invalid_argument("Column \"" + cond.column + "\" does not exist in table.");
            }

            currentGroup.push_back(cond);
            i += 3;

            if (i < tokens.size() && tokens[i] == AND)
                i++;
            if (i < tokens.size() && tokens[i] == OR) {
                groups.push_back(currentGroup);
                currentGroup.clear();
                i++;
            }
        } else {
            throw invalid_argument("Not enough arguments to form a condition.");
        }
    }
    if (!currentGroup.empty())
        groups.push_back(currentGroup);
    return groups;
}

// Overload that updates only the specified column.
void Table::updateValueByCondition(const string &colName, const string &oldValue, const string &newValue,const vector<vector<Condition>> &conditionGroups) {
    int colIndex = -1;
    for (int i = 0; i < headers.size(); i++) {
        if (headers[i] == colName) {
            string dataType = columnMeta[colName].first;
            if( oldValue == "null" || !validateValue(oldValue,dataType) ){
                throw ("mismatch_error: Value "+ oldValue +" is not valid for column " + colName + " of type " + dataType + ".");
            } else if (newValue == "null" || !validateValue(newValue,dataType) ){
                throw ("mismatch_error: Value "+ newValue +" is not valid for column " + colName + " of type " + dataType + ".");
            }
            colIndex = i;
            break;
        }
    }
    if (colIndex == -1) {
        throw invalid_argument("Column: \"" + colName + "\" not found.");
    }
    if (colIndex == primaryKeyIndex && dataMap.find(newValue) != dataMap.end()) { 
        throw ("Constraint Error: Primary Key " + newValue + " already exists. Skipping Updation.");
    }
    int updateCount = 0;
    for (auto &pair : dataMap) {
        // Evaluate advanced conditions on the row.
        if (evaluateAdvancedConditions(pair.second, conditionGroups)) {
            int idx = colIndex - 1;
            if (idx < pair.second.values.size() && pair.second.values[idx] == oldValue) {
                pair.second.values[idx] = newValue;
                updateCount++;
            }
        }
    }
    if (updateCount > 0){
        cout <<"res : " << updateCount << " row(s) updated successfully." << endl;
        unsavedChanges = true;
    }   
    else
        throw invalid_argument("Logic ERR: No matching rows found with " + colName + " = " + oldValue + " under the given conditions." );

}

// Overload that updates across all columns (except primary key) where any cell equals oldValue.
void Table::updateValueByCondition(const string &oldValue, const string &newValue,const vector<vector<Condition>> &conditionGroups) {
    int updateCount = 0;
    for (auto &pair : dataMap) {
        if (evaluateAdvancedConditions(pair.second, conditionGroups)) {
            // For each column (except primary key), update if the cell equals oldValue.
            for (int i = 0; i < headers.size(); i++) {
                if(i == primaryKeyIndex && dataMap.find(newValue) != dataMap.end()){
                    continue;
                }
                int idx = i - 1;
                if (idx < pair.second.values.size() && pair.second.values[idx] == oldValue) {
                    pair.second.values[idx] = newValue;
                    updateCount++;
                }
            }
        }
    }
    if (updateCount > 0){
        cout <<"Response: " << updateCount << " row(s) updated successfully." << endl;
        unsavedChanges = true;
    }
    else
        cout << "No matching rows found with " << oldValue << " under the given conditions." << endl;
}

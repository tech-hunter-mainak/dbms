#include "row.cpp"
#include "library.cpp"  // Or your other necessary headers


struct Condition {
    string column;
    string op;     // Operator (e.g., =, >, <, <=, >=, / for not equal)
    string value;
};
extern string currentTable;

string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t"); // Finds the first character that is not a space or tab
    if(start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t"); // Finds the last such character that is not a space or tab
    return s.substr(start, end - start + 1); // ✅ The result includes the character at start_index,❌ But does NOT use an end_index.
}
// Helper for comparing two values based on an operator.
bool compareValues(const string &actual, const string &op, const string &expected);

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
            values.push_back("");
        } else {
            values.push_back(item);
        }
    }
    if(item.empty()){
        values.push_back("");
    }
    return values;
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
            if (!meta.second.empty())
                headerLine += "(" + meta.second + ")";
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
            string tName;
            int rowCount;
            if (iss >> tName >> rowCount)
                metadata[tName] = rowCount;
        }
        metaIn.close();
        return metadata;
    }

    // Updates the metadata file with the row count for this table.
    void updateTableMetadata() {
        string metaFileName = "table_metadata.txt";
        map<string, int> metadata = readTableMetadata();
        metadata[tableName] = static_cast<int>(rowOrder.size());
        ofstream metaOut(metaFileName);
        for (const auto &p : metadata) {
            metaOut << p.first << " " << p.second << "\n";
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
                            if (c == "PRIMARY_KEY") {
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
        if (values.empty()) {
            throw invalid_argument("INSERT -> missing values/delimitters(,).");
        }
        if (values.size() > headers.size()) {
            cerr << "invalid_argument: Number of values (" << values.size() << ") does not match number of columns (" << headers.size() << ")." << endl;
            throw ("");
        }
        for (size_t i = 0; i < values.size(); i++) {
            string colName = headers[i];
            string expectedType = columnMeta[colName].first;
            string consStr = columnMeta[colName].second;
            unordered_set<string> consSet = parseConstraints(consStr);
        
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
                        throw runtime_error("Constraint Error: Duplicate value '" + values[i] +
                                            "' found in UNIQUE column '" + colName + "'.");
                    }
                }
            }
        
            // AUTO_INCREMENT is handled for primary key (and optionally other columns) as in section 3.
            if (consSet.count("AUTO_INCREMENT") || consSet.count("PRIMARY_KEY")) {
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
                throw runtime_error("Mismatch Error: Value \"" + values[i] + "\" is not valid for column \"" 
                                       + colName + "\" of type " + expectedType + ".");
            }
        }
        // Check primary key constraint
        string pkValue = values[primaryKeyIndex];
        if (dataMap.find(pkValue) != dataMap.end()) {
            cerr << "Constraint Error: Primary Key " << pkValue << " already exists." << endl;
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
        cout << "res: Commit successful." << endl;
    }
    void rollbackTransaction() {
        if(unsavedChanges){
            retrieveData();
        }else{
            cerr << "WARNING: No changes made to table." << endl;
        }
        cout << "res: Rollback successful." << endl;
    }
    void describe() {
        // Print a header for the description.
        cout << "Table: " << currentTable << "\n";
        cout << "-------------------------------------------------\n";
        cout << setw(20) << left << "Column Name" 
             << setw(15) << left << "Data Type"
             << "Constraints" << "\n";
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
            string dataType = (metaIt != columnMeta.end()) ? metaIt->second.first : "*EMPTY";
            string constraints = (metaIt != columnMeta.end()) ? metaIt->second.second : "*EMPTY";
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
            cerr << "Syntax Error: SHOW -> No viable arguments provided.\n";
            return;
        }
        
        // --- Check for LIKE clause ---
        bool likeMode = false;
        string likePattern;  // prefix pattern for LIKE search (without trailing '%')
        // Look for the token "LIKE" (case-insensitive)
        for (size_t i = 0; i < tokens.size(); i++) {
            string t = tokens[i];
            string upperT = t;
            transform(upperT.begin(), upperT.end(), upperT.begin(), ::toupper);
            if (upperT == "LIKE") {
                likeMode = true;
                if (i + 1 < tokens.size()) {
                    likePattern = trim(tokens[i + 1]);
                    if (!likePattern.empty() && ((likePattern.front() == '"' && likePattern.back() == '"') ||
                                                 (likePattern.front() == '\'' && likePattern.back() == '\'')))
                        likePattern = likePattern.substr(1, likePattern.size() - 2);
                    if (!likePattern.empty() && likePattern.back() == '%')
                        likePattern.pop_back();
                }
                // Erase the LIKE token and its argument.
                tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
                break;
            }
        }
        
        // --- Determine the mode based on the first token ---
        // If the first token is "*" then we show all columns.
        if (tokens[0] == "*") {
            tokens.erase(tokens.begin()); // remove "*"
            // Check for an optional WHERE clause.
            bool whereClause = false;
            vector<string> condTokens;
            if (!tokens.empty() && (tokens[0] == "where" || tokens[0] == "WHERE")) {
                whereClause = true;
                tokens.erase(tokens.begin()); // remove "where"
                condTokens = tokens;  // remaining tokens form the WHERE clause
            }
            
            size_t nCols = headers.size();
            vector<size_t> colWidths(nCols, 0);
            // Update widths based on header lengths.
            for (size_t i = 0; i < nCols; i++) {
                colWidths[i] = headers[i].length();
            }
            
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
            
            // Update column widths based on row content.
            for (const auto &pk : rowOrder) {
                vector<string> cells = getRowCells(pk);
                for (size_t i = 0; i < nCols; i++) {
                    colWidths[i] = max(colWidths[i], cells[i].length());
                }
            }
            
            auto center = [&](const string &s, size_t width) -> string {
                if (s.length() >= width)
                    return s;
                size_t leftPadding = (width - s.length()) / 2;
                size_t rightPadding = width - s.length() - leftPadding;
                return string(leftPadding, ' ') + s + string(rightPadding, ' ');
            };
            
            // Print header row.
            for (size_t i = 0; i < nCols; i++) {
                cout << center(headers[i], colWidths[i]);
                if (i != nCols - 1)
                    cout << " | ";
            }
            cout << "\n";
            // Print divider.
            for (size_t i = 0; i < nCols; i++) {
                cout << string(colWidths[i], '-');
                if (i != nCols - 1)
                    cout << "-+-";
            }
            cout << "\n";
            
            // Lambda for filtering by LIKE (applies to all string columns).
            auto rowMatchesLike = [&](const vector<string>& cells) -> bool {
                for (size_t i = 0; i < nCols; i++) {
                    // Optionally, skip auto-generated primary key column.
                    // if (isAutoPrimaryKey && i == 0)
                    //     continue;
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
            
            // Lambda for WHERE filtering.
            auto rowMatchesWhere = [&](const vector<string>& cells) -> bool {
                // Reconstruct a Row object from cells.
                Row row;
                // if (isAutoPrimaryKey) {
                //     row.id = cells[0];
                //     for (size_t i = 1; i < cells.size(); i++)
                //         row.values.push_back(cells[i]);
                // } else {
                //     row.id = cells[primaryKeyIndex];
                //     for (size_t i = 0; i < cells.size(); i++) {
                //         if ((int)i == primaryKeyIndex)
                //             continue;
                //         row.values.push_back(cells[i]);
                //     }
                // }
                row.id = cells[primaryKeyIndex];
                for (size_t i = 0; i < cells.size(); i++) {
                    if ((int)i == primaryKeyIndex)
                        continue;
                    row.values.push_back(cells[i]);
                }
                return evaluateAdvancedConditions(row, parseAdvancedConditions(condTokens));
            };
            
            // Print rows.
            for (const auto &pk : rowOrder) {
                vector<string> cells = getRowCells(pk);
                if (likeMode && !rowMatchesLike(cells))
                    continue;
                if (!condTokens.empty() && !rowMatchesWhere(cells))
                    continue;
                for (size_t i = 0; i < nCols; i++) {
                    cout << setw(colWidths[i]) << left << cells[i];
                    if (i != nCols - 1)
                        cout << " | ";
                }
                cout << "\n";
            }
        }
        // HEAD and LIMIT branches remain unchanged...
        else if (tokens[0] == HEAD) {
            int defaultLimit = 5;
            // [Your existing HEAD branch code, using similar printing lambdas]
        }
        else if (tokens[0] == LIMIT) {
            // [Your existing LIMIT branch code]
        }
        else {
            // This branch handles "show <col1>,<col2>,... [where/LIKE <conditions>]"
            // Instead of joining tokens, treat each token before a WHERE/LIKE as a separate column spec.
            vector<string> selectedColumns;
            int clauseIndex = -1;
            for (int i = 0; i < tokens.size(); i++) {
                string upperToken = tokens[i];
                transform(upperToken.begin(), upperToken.end(), upperToken.begin(), ::toupper);
                if (upperToken == "WHERE" || upperToken == "LIKE") {
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
            
            // Determine if there's an extra clause.
            vector<string> extraTokens;
            if (clauseIndex != -1) {
                extraTokens.assign(tokens.begin() + clauseIndex + 1, tokens.end());
            }
            
            // If the clause is LIKE, then set likeMode.
            if (clauseIndex != -1) {
                string firstClause = tokens[clauseIndex];
                string upperClause = firstClause;
                transform(upperClause.begin(), upperClause.end(), upperClause.begin(), ::toupper);
                if (upperClause == "LIKE") {
                    likeMode = true;
                    if (!extraTokens.empty()) {
                        likePattern = trim(extraTokens[0]);
                        if (!likePattern.empty() &&
                           ((likePattern.front() == '"' && likePattern.back() == '"') ||
                            (likePattern.front() == '\'' && likePattern.back() == '\'')))
                            likePattern = likePattern.substr(1, likePattern.size() - 2);
                        if (!likePattern.empty() && likePattern.back() == '%')
                            likePattern.pop_back();
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
                    cout << "Column \"" << colName << "\" not found.\n";
                    return;
                }
                colIndices.push_back(colIndex);
            }
            
            // Compute dynamic widths for selected columns.
            size_t nCols = colIndices.size();
            vector<size_t> colWidths(nCols, 0);
            for (size_t i = 0; i < nCols; i++) {
                colWidths[i] = headers[colIndices[i]].length();
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
                for (size_t i = 0; i < nCols; i++) {
                    string cell = getCellValue(it->second, colIndices[i]);
                    colWidths[i] = max(colWidths[i], cell.length());
                }
            }
            
            auto center = [&](const string &s, size_t width) -> string {
                if (s.length() >= width)
                    return s;
                size_t leftPadding = (width - s.length()) / 2;
                size_t rightPadding = width - s.length() - leftPadding;
                return string(leftPadding, ' ') + s + string(rightPadding, ' ');
            };
            
            // Print header row (centered).
            for (size_t i = 0; i < selectedColumns.size(); i++) {
                cout << center(selectedColumns[i], colWidths[i]);
                if (i != selectedColumns.size() - 1)
                    cout << " | ";
            }
            cout << "\n";
            // Print divider.
            for (size_t i = 0; i < selectedColumns.size(); i++) {
                cout << string(colWidths[i], '-');
                if (i != selectedColumns.size() - 1)
                    cout << "-+-";
            }
            cout << "\n";
            
            // Parse WHERE conditions if extraTokens exist.
            vector<vector<Condition>> conditionGroups;
            if (!extraTokens.empty())
                conditionGroups = parseAdvancedConditions(extraTokens);
            
            // For LIKE filtering on selected columns.
            auto rowMatchesLike = [&](const Row &row) -> bool {
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
                if (likeMode && !rowMatchesLike(it->second))
                    continue;
                for (size_t i = 0; i < nCols; i++) {
                    string cell = getCellValue(it->second, colIndices[i]);
                    cout << setw(colWidths[i]) << left << cell;
                    if (i != nCols - 1)
                        cout << " | ";
                }
                cout << "\n";
            }
        }
    }
    
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
    cout << "res: Column \"" << colName << "\" deleted successfully." << endl;
    unsavedChanges = true;
}
void Table::deleteRowsByAdvancedConditions(const vector<vector<Condition>> &groups) {
    vector<string> rowsToDelete;
    // --- Step 1: Validate column names before doing anything ---
    // for (const auto &group : groups) {
    //     for (const auto &cond : group) {
    //         bool columnExists = false;
    //         for (const auto &header : headers) {
    //             if (header == cond.column) {
    //                 columnExists = true;
    //                 break;
    //             }
    //         }
    //         if (!columnExists) {
    //             cerr << "Logic Error: Column \"" + cond.column + "\" not found." << endl;
    //             cout <<"Response: " << rowsToDelete.size() << " row(s) affected." << endl;
    //             return;
    //         }
    //     }
    // }
    // Iterate over each row.
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
    cout <<"res: " << rowsToDelete.size() << " row(s) affected." << endl;
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
            if (colIndex == primaryKeyIndex)
                actual = row.id;
            else {
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

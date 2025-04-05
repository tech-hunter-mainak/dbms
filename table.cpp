#include "row.cpp"
#include "library.cpp"  // Or your other necessary headers


struct Condition {
    string column;
    string op;     // Operator (e.g., =, >, <, <=, >=, / for not equal)
    string value;
};

// Parses tokens into condition groups. Each OR group is a vector of Conditions.
vector<vector<Condition>> parseAdvancedConditions(const vector<string>& tokens);

// Helper for comparing two values based on an operator.
bool compareValues(const string &actual, const string &op, const string &expected);


// (Other helper functions such as extractValues, trim, etc. can be declared here or in a utilities header)
// Helper function to trim spaces.
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    if (start == string::npos || end == string::npos)
        return "";
    return s.substr(start, end - start + 1);
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
    else if (dataType == "BIINT") {
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
    else if (dataType == "BIDOUBLE") {
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
        // For VARCHAR or STRING, ensure it's not empty.
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
    // size_t start = command.find('(');
    // size_t end = command.find(')');
    // if (start == string::npos || end == string::npos || start > end) {
    //     cout << "Invalid command format. Use: insert values (\"value1\", \"value2\", ...)\n";
    //     return values;
    // }
    // string data = command.substr(start + 1, end - start - 1); // Extract inside ()
    stringstream ss(command);
    string item;
    while (getline(ss, item, ',')) {
        // Trim spaces and remove quotes.
        item.erase(remove(item.begin(), item.end(), '"'), item.end());
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        // cout<<item<<endl; debugging statement.
        if(item.empty()){
            values.push_back("null");
        } else {
            values.push_back(item);
        }
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


    void writeToFile() {
        ofstream file(filename);
        if (!file.is_open())
        {
            cout << "Error opening file for commit." << endl;
            return;
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
        for (const auto &id : rowOrder)
        {
            auto it = dataMap.find(id);
            if (it != dataMap.end()) {
                file << it->first;  // Write the ID.
                for (const auto &val : it->second.values)
                {
                    file << "," << val;
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
      : tableName(tName), filename(tName + ".csv"), columnWidth(15)
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
            cout << "Error occured while opening " << filename << " check tablename." << "!\n";
            return;
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
                        string allConstraints;
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
                } else {
                    cout << " PRIMARY_KEY detected in header with id"<< primaryKeyIndex << endl;
                }
            } else {
                if (!rowValues.empty()) {
                    // debug print the row values if needed
                    // for (const auto& pair : dataMap) {
                    //     cout << "- " << pair.first << endl;
                    // }
                    // Ensure that the row has as many values as there are headers.
                    if (rowValues.size() < headers.size()) {
                        cout << "Row skipped: insufficient number of columns." << endl;
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
    
    // Print all table data in a formatted layout.
    void printData() {
        if (headers.empty()) {
            cout << "No data to display.\n";
            return;
        }
        // Print header row.
        for (const auto &header : headers) {
            cout << setw(columnWidth) << left << header;
        }
        cout << "\n" << string(headers.size() * columnWidth, '-') << "\n";
        // Print each row in insertion order.
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
    /* DONE */
    // Insert a row given a command string (which includes parenthesized values).
    void insertRow(const string &command) {
        vector<string> values = extractValues(command);
        if (values.empty()) {
            cout << "Syntax Error: values are not entered correctly" << endl;
            return;
        }
        if (values.size() > headers.size()) {
            cout << "Error: Number of values (" << values.size() 
                 << ") does not match number of columns (" << headers.size() << ")." << endl;
            return;
        }
        // Validate values using columnMeta.
        for (size_t i = 0; i < values.size(); i++) {
            string colName = headers[i];
            string expectedType = columnMeta[colName].first;
            if (!validateValue(values[i], expectedType)) {
                cout << "Error: Value \"" << values[i] << "\" is not valid for column \"" 
                     << colName << "\" of type " << expectedType << "." << endl;
                return;
            }
        }
        // The primary_key constraint is assumed to be row ID. this skips insertion.
        string id = values[primaryKeyIndex];
        if (dataMap.find(id) != dataMap.end()) {
            cout << "Constraint Error: Primary Key " << id << " already exists. Skipping insertion." << endl;
            return;
        }
        vector<string> rowValues;
        for (size_t i = 0; i < values.size(); i++) {
            if (i == (size_t)primaryKeyIndex)
                continue;
            rowValues.push_back(values[i]);
        }
        dataMap[id] = Row(id, rowValues);
        rowOrder.push_back(id);
        cout << "Data inserted successfully into table. into id"<< id << endl;
    }
    
    // Delete a row with a given ID.
    void deleteRow(const string &id) {
        auto it = dataMap.find(id);
        if (it != dataMap.end()) {
            dataMap.erase(it);
            rowOrder.erase(remove(rowOrder.begin(), rowOrder.end(), id), rowOrder.end());
            cout << "Row deleted successfully from in-memory data." << endl;
        }
        else {
            cout << "ID not found in in-memory data." << endl;
        }
    }
    
    // Clear all rows from the table (keeping headers intact).
    void cleanTable() {
        dataMap.clear();
        rowOrder.clear();
    }
    
    // // Update a value in a given column for rows that match a condition.
    // void updateValueByCondition(const string &colName, const string &oldValue, const string &newValue) {
    //     int colIndex = -1;
    //     for (int i = 0; i < headers.size(); i++) {
    //         if (headers[i] == colName) {
    //             colIndex = i;
    //             break;
    //         }
    //     }
    //     if (colIndex == -1) {
    //         cout << "Column \"" << colName << "\" not found." << endl;
    //         return;
    //     }
    //     if (colIndex == primaryKeyIndex && dataMap.find(newValue) != dataMap.end()) { 
    //         cout << "Constraint Error: Primary Key " << newValue << " already exists. Skipping Updation." << endl;
    //         return; 
    //     }

    //     int updateCount = 0;
    //     for (auto &pair : dataMap) {
    //         int index = colIndex - 1;
    //         if (index < pair.second.values.size() && pair.second.values[index] == oldValue) {
    //             pair.second.values[index] = newValue;
    //             updateCount++;
    //         }
    //     }
    //     if (updateCount > 0)
    //         cout << updateCount << " row(s) updated successfully." << endl;
    //     else
    //         cout << "No rows found with " << colName << " = " << oldValue << endl;
    // }
        
    void commitTransaction() {
        writeToFile();
        cout << "Commit successful. Changes written to file: " 
             << fs::absolute(filename).string() << endl;
        updateTableMetadata();
    }
    
    void rollbackTransaction() {
        retrieveData();
        cout << "Rollback successful. In-memory data restored from file." << endl;
    }
    
    void show(const string &params) {
        // Tokenize parameters.
        istringstream iss(params);
        vector<string> tokens;
        string token;
        while (iss >> token)
            tokens.push_back(token);
        if (tokens.empty()) {
            cout << "No arguments provided for SHOW command.\n";
            return;
        }
        
        auto printFullHeader = [&]() {
            for (const auto &hdr : headers)
                cout << setw(columnWidth) << left << hdr;
            cout << "\n" << string(headers.size() * columnWidth, '-') << "\n";
        };
        
        auto printColumnHeader = [&](int colIndex) {
            cout << setw(columnWidth) << left << headers[colIndex] << "\n";
            cout << string(columnWidth, '-') << "\n";
        };
        
        if (tokens[0] == "*") {
            if (tokens.size() > 1 && tokens[1] == "where") {
                vector<string> condTokens(tokens.begin() + 2, tokens.end());
                // Use the advanced parser here.
                vector<vector<Condition>> conditionGroups = parseAdvancedConditions(condTokens);
                printFullHeader();
                for (const auto &id : rowOrder) {
                    auto it = dataMap.find(id);
                    if (it != dataMap.end() && evaluateAdvancedConditions(it->second, conditionGroups)) {
                        cout << setw(columnWidth) << left << it->first;
                        for (const auto &val : it->second.values)
                            cout << setw(columnWidth) << left << val;
                        cout << "\n";
                    }
                }
            } else {
                printFullHeader();
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
        else if (tokens[0] == HEAD) {
            int defaultLimit = 5;
            printFullHeader();
            int count = 0;
            for (const auto &id : rowOrder) {
                if (count >= defaultLimit)
                    break;
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
        else if (tokens[0] == LIMIT) {
            if (tokens.size() < 2) {
                cout << "Missing number for LIMIT command.\n";
                return;
            }
            bool fromBottom = false;
            string numToken = tokens[1];
            if (numToken[0] == TILDE) {
                fromBottom = true;
                numToken = numToken.substr(1);
            }
            int limit = 0;
            try {
                limit = stoi(numToken);
            } catch (...) {
                cout << "Invalid number for LIMIT command.\n";
                return;
            }
            if (limit <= 0) {
                cout << "Limit must be a positive integer.\n";
                return;
            }
            printFullHeader();
            int total = rowOrder.size();
            if (!fromBottom) {
                int count = 0;
                for (const auto &id : rowOrder) {
                    if (count >= limit)
                        break;
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
        else {
            // Assume first token is a column name.
            string colName = tokens[0];
            if (!colName.empty() && colName.front() == '"' && colName.back() == '"')
                colName = colName.substr(1, colName.size() - 2);
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
            if (tokens.size() > 1 && tokens[1] == "where") {
                vector<string> condTokens(tokens.begin() + 2, tokens.end());
                vector<vector<Condition>> conditionGroups = parseAdvancedConditions(condTokens);
                printColumnHeader(colIndex);
                for (const auto &id : rowOrder) {
                    auto it = dataMap.find(id);
                    if (it != dataMap.end() && evaluateAdvancedConditions(it->second, conditionGroups)) {
                        string cell;
                        if (colIndex == 0)
                            cell = it->first;
                        else {
                            int idx = colIndex - 1;
                            if (idx < it->second.values.size())
                                cell = it->second.values[idx];
                        }
                        cout << setw(columnWidth) << left << cell << "\n";
                    }
                }
            } else {
                printColumnHeader(colIndex);
                for (const auto &id : rowOrder) {
                    auto it = dataMap.find(id);
                    if (it != dataMap.end()) {
                        string cell;
                        if (colIndex == 0)
                            cell = it->first;
                        else {
                            int idx = colIndex - 1;
                            if (idx < it->second.values.size())
                                cell = it->second.values[idx];
                        }
                        cout << setw(columnWidth) << left << cell << "\n";
                    }
                }
            }
        }
    }
    
};
bool Table::hasRow(const string &id) {
    cout << "Checking if ID exists: " << id << endl;
    cout << "Current IDs in dataMap:\n";
    for (const auto& pair : dataMap) {
        cout << "- " << pair.first << endl;
    }

    return dataMap.find(id) != dataMap.end();
}

bool Table::hasColumn(const string &colName) {
    // Check if the column exists in the headers.
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
    if (colIndex == -1) {
        cout << "Column \"" << colName << "\" not found." << endl;
        return;
    }
    if (colIndex == primaryKeyIndex) {  // Prevent deletion of primary key.
        cout << "Primary key column cannot be deleted." << endl;
        return;
    }
    // Remove from headers and metadata.
    headers.erase(headers.begin() + colIndex);
    columnMeta.erase(colName);
    // Remove the corresponding value from each row.
    for (auto &pair : dataMap) {
        if (pair.second.values.size() >= colIndex)
            pair.second.values.erase(pair.second.values.begin() + (colIndex - 1));
    }
    cout << "Column \"" << colName << "\" deleted successfully." << endl;
}
void Table::deleteRowsByAdvancedConditions(const vector<vector<Condition>> &groups) {
    vector<string> rowsToDelete;
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
    cout << rowsToDelete.size() << " row(s) deleted based on conditions." << endl;
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
            if (colIndex == 0)
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
vector<vector<Condition>> parseAdvancedConditions(const vector<string>& tokens) {
    // so logically speaking for this cond1 AND cond2 OR cond3 AND cond4
    // this is how it gets stored in groups: 
    /*[
        [cond1, cond2],
        [cond3, cond4]
      ] 
    */
    vector<vector<Condition>> groups;
    vector<Condition> currentGroup;
    auto trimQuotes = [](const string &s) -> string { // this is a lambda function kind of.
        string trimmed = trimStr(s);
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
            currentGroup.push_back(cond);
            i += 3;
            // Skip an "and" token if present.
            if (i < tokens.size() && tokens[i] == AND)
                i++;
            // If an "or" token is encountered, finish the current group.
            if (i < tokens.size() && tokens[i] == OR) {
                groups.push_back(currentGroup);
                currentGroup.clear();
                i++;
            }
        } else {
            cerr << "ERROR: Something wrong in conditions"<<endl; 
            break;
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
            colIndex = i;
            break;
        }
    }
    if (colIndex == -1) {
        cout << "Column \"" << colName << "\" not found." << endl;
        return;
    }
    if (colIndex == primaryKeyIndex && dataMap.find(newValue) != dataMap.end()) { 
        cout << "Constraint Error: Primary Key " << newValue << " already exists. Skipping Updation." << endl;
        return; 
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
    if (updateCount > 0)
        cout << updateCount << " row(s) updated successfully." << endl;
    else
        cout << "No matching rows found with " << colName << " = " << oldValue << " under the given conditions." << endl;
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
    if (updateCount > 0)
        cout << updateCount << " row(s) updated successfully." << endl;
    else
        cout << "No matching rows found with " << oldValue << " under the given conditions." << endl;
}


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

// Helper function to trim whitespace from both ends of a string.
std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
    auto end = s.end();
    if(start != s.end()){
        do {
            end--;
        } while (std::distance(start, end) > 0 && std::isspace(*end));
    }
    return std::string(start, end + 1);
}

// Utility function to get the DBMS folder on the Desktop
std::string getDesktopDBMSPath() {
    const char* homeDir = std::getenv(
    #ifdef _WIN32
        "USERPROFILE" // Windows
    #else
        "HOME"        // macOS/Linux
    #endif
    );
    if (!homeDir) {
        std::cerr << "Error: Could not retrieve home directory.\n";
        return "";
    }
    return (fs::path(homeDir) / "Desktop" / "DBMS").string();
}

// Class for managing rows of a table
class Row {
public:
    std::vector<std::string> values;

    Row(const std::vector<std::string>& values) : values(values) {}

    Row(const std::string& csvLine) {
        std::stringstream ss(csvLine);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            values.push_back(trim(cell));
        }
    }

    void updateValue(size_t index, const std::string& newValue) {
        if (index < values.size()) {
            values[index] = newValue;
        }
    }

    std::string toCSV() const {
        std::ostringstream oss;
        for (size_t i = 0; i < values.size(); ++i) {
            oss << values[i];
            if (i < values.size() - 1)
                oss << ",";
        }
        return oss.str();
    }
};

// Class for managing tables
class Table {
private:
    std::string filePath;
    std::vector<std::string> schema;

    // Ensure the filePath has a ".csv" extension
    void ensureCsvExtension() {
        fs::path p(filePath);
        if (p.extension() != ".csv") {
            filePath += ".csv";
        }
    }

public:
    // Constructor receives the database name and table name.
    Table(const std::string& dbName, const std::string& tableName, const std::vector<std::string>& schema = {})
        : schema(schema)
    {
        const char* homeDir = std::getenv(
        #ifdef _WIN32
            "USERPROFILE" // Windows
        #else
            "HOME"        // macOS/Linux
        #endif
        );
        if (!homeDir) {
            std::cerr << "Error: Could not retrieve HOME environment variable.\n";
            return;
        }
        fs::path dbPath = fs::path(homeDir) / "Desktop" / "DBMS" / dbName;
        if (!fs::exists(dbPath)) {
            std::cerr << "Error: Database directory does not exist: " << dbPath << "\n";
            return;
        }
        filePath = (dbPath / tableName).string();
        ensureCsvExtension();
    }

    // Create a new table file (overwrite if exists)
    void create() {
        std::ofstream file(filePath, std::ios::trunc);
        if (!file.is_open()) {
            std::cerr << "Failed to create table: " << filePath << "\n";
            return;
        }
        if (!schema.empty()) {
            std::vector<std::string> trimmedSchema;
            for (const auto& col : schema) {
                trimmedSchema.push_back(trim(col));
            }
            file << Row(trimmedSchema).toCSV() << "\n";
        }
        std::cout << "Table created: " << filePath << "\n";
    }

    // Insert a new row into the table
    void insertRow(const Row& row) {
        std::ofstream file(filePath, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Failed to insert row into table: " << filePath << "\n";
            return;
        }
        file << row.toCSV() << "\n";
        std::cout << "Row inserted into table: " << filePath << "\n";
    }

    // Display all rows in the table
    void selectRows() const {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to read table: " << filePath << "\n";
            return;
        }
        std::string line;
        std::cout << "Contents of table: " << filePath << "\n";
        while (std::getline(file, line)) {
            std::cout << line << "\n";
        }
    }

    // Get the file path (useful for update, delete, and drop commands)
    const std::string& getFilePath() const {
        return filePath;
    }
};

// Class for managing databases
class Database {
private:
    std::string name;
    fs::path dbmsPath;
public:
    Database() = default;

    Database(const std::string& dbName) : name(dbName) {
        const char* homeDir = std::getenv(
        #ifdef _WIN32
            "USERPROFILE"
        #else
            "HOME"
        #endif
        );
        if (!homeDir) {
            std::cerr << "Error: Could not retrieve home directory.\n";
            return;
        }
        dbmsPath = fs::path(homeDir) / "Desktop" / "DBMS";
    }

    const std::string& getName() const {
        return name;
    }

    void create() {
        if (name.empty()) {
            std::cerr << "Error: Database name is empty.\n";
            return;
        }
        if (!fs::exists(dbmsPath)) {
            if (!fs::create_directory(dbmsPath)) {
                std::cerr << "Error: Failed to create DBMS directory at " << dbmsPath << "\n";
                return;
            }
            std::cout << "DBMS folder created at: " << dbmsPath << "\n";
        }
        fs::path databasePath = dbmsPath / name;
        if (fs::exists(databasePath)) {
            std::cout << "Database already exists: " << databasePath << "\n";
        } else if (fs::create_directory(databasePath)) {
            std::cout << "Database created: " << databasePath << "\n";
        } else {
            std::cerr << "Failed to create database: " << databasePath << "\n";
        }
    }

    Table createTable(const std::string& tableName, const std::vector<std::string>& schema) {
        Table table(name, tableName, schema);
        table.create();
        return table;
    }
};

// Parser class for interpreting commands robustly with full SET and WHERE support
class Parser {
public:
    void parseCommand(const std::string& command, Database& db) {
        if(command.empty()){
            std::cerr << "Empty command received.\n";
            return;
        }
        std::istringstream iss(command);
        std::string action;
        iss >> action;
        action = trim(action);

        if (action == "CREATE") {
            std::string type;
            iss >> type;
            type = trim(type);
            if (type == "TABLE") {
                std::string tableName;
                if (!(iss >> tableName)) {
                    std::cerr << "Missing table name in CREATE TABLE command.\n";
                    return;
                }
                tableName = trim(tableName);
                std::string schemaStr;
                std::getline(iss, schemaStr);
                schemaStr = trim(schemaStr);
                std::vector<std::string> schema;
                if (!schemaStr.empty()) {
                    std::istringstream schemaStream(schemaStr);
                    std::string column;
                    while (std::getline(schemaStream, column, ',')) {
                        column = trim(column);
                        if (!column.empty())
                            schema.push_back(column);
                    }
                }
                db.createTable(tableName, schema);
            }
            else if (type == "DATABASE") {
                std::string dbName;
                if (!(iss >> dbName)) {
                    std::cerr << "Missing database name in CREATE DATABASE command.\n";
                    return;
                }
                dbName = trim(dbName);
                Database newDb(dbName);
                newDb.create();
            }
            else {
                std::cerr << "Invalid CREATE command type: " << type << "\n";
            }
        }
        else if (action == "INSERT") {
            std::string tableName;
            if (!(iss >> tableName)) {
                std::cerr << "Missing table name in INSERT command.\n";
                return;
            }
            tableName = trim(tableName);
            std::string values;
            std::getline(iss, values);
            values = trim(values);
            if (values.empty()) {
                std::cerr << "No values provided in INSERT command.\n";
                return;
            }
            std::istringstream valueStream(values);
            std::vector<std::string> rowValues;
            std::string value;
            while (std::getline(valueStream, value, ',')) {
                rowValues.push_back(trim(value));
            }
            Table table(db.getName(), tableName, {});
            table.insertRow(Row(rowValues));
        }
        else if (action == "SELECT") {
            std::string tableName;
            if (!(iss >> tableName)) {
                std::cerr << "Missing table name in SELECT command.\n";
                return;
            }
            tableName = trim(tableName);
            
            // Check for an optional WHERE clause.
            std::string whereClause;
            std::getline(iss, whereClause);
            whereClause = trim(whereClause);
            
            Table table(db.getName(), tableName, {});
            std::ifstream file(table.getFilePath());
            if (!file.is_open()) {
                std::cerr << "Failed to read table: " << table.getFilePath() << "\n";
                return;
            }
            
            // Read header row.
            std::string headerLine;
            if (!std::getline(file, headerLine)) {
                std::cerr << "Table is empty.\n";
                return;
            }
            Row headerRow(headerLine);
            auto header = headerRow.values;
            
            // Variables to hold the condition details.
            int conditionIndex = -1;
            std::string conditionValue;
            
            // If a WHERE clause is provided, parse it.
            if (!whereClause.empty()) {
                // If the clause starts with "WHERE", remove it.
                if (whereClause.find("WHERE") == 0)
                    whereClause = trim(whereClause.substr(5));
                // Expecting format: column=value
                size_t eqPos = whereClause.find('=');
                if (eqPos == std::string::npos) {
                    std::cerr << "Invalid WHERE clause format in SELECT. Expected column=value\n";
                    return;
                }
                std::string conditionColumn = trim(whereClause.substr(0, eqPos));
                conditionValue = trim(whereClause.substr(eqPos + 1));
                // Find the index of the condition column in the header.
                for (size_t i = 0; i < header.size(); i++) {
                    if (header[i] == conditionColumn) {
                        conditionIndex = i;
                        break;
                    }
                }
                if (conditionIndex == -1) {
                    std::cerr << "Condition column '" << conditionColumn << "' not found in table header.\n";
                    return;
                }
            }
            
            // Print the header row.
            std::cout << headerRow.toCSV() << "\n";
            
            // Read and display the rows.
            std::string line;
            while (std::getline(file, line)) {
                Row row(line);
                // If a condition is provided, only print matching rows.
                if (conditionIndex != -1) {
                    if (row.values.size() <= static_cast<size_t>(conditionIndex))
                        continue; // Skip malformed row.
                    if (row.values[conditionIndex] == conditionValue) {
                        std::cout << row.toCSV() << "\n";
                    }
                } else {
                    // No condition; print all rows.
                    std::cout << row.toCSV() << "\n";
                }
            }
        }

        else if (action == "UPDATE") {
            std::string tableName;
            if (!(iss >> tableName)) {
                std::cerr << "Missing table name in UPDATE command.\n";
                return;
            }
            tableName = trim(tableName);
            std::string restOfCommand;
            std::getline(iss, restOfCommand);
            restOfCommand = trim(restOfCommand);
            if (restOfCommand.empty() || restOfCommand.find("SET") != 0) {
                std::cerr << "Missing SET clause in UPDATE command.\n";
                return;
            }
            // Remove "SET" from the beginning
            restOfCommand = trim(restOfCommand.substr(3));
            // Check if a WHERE clause exists
            std::string updateClause, whereClause;
            size_t wherePos = restOfCommand.find("WHERE");
            if (wherePos != std::string::npos) {
                updateClause = trim(restOfCommand.substr(0, wherePos));
                whereClause = trim(restOfCommand.substr(wherePos + 5));
            } else {
                updateClause = restOfCommand;
            }
            // Parse updateClause: expecting "column=new_value"
            size_t eqPos = updateClause.find('=');
            if (eqPos == std::string::npos) {
                std::cerr << "Invalid SET clause format. Expected column=new_value\n";
                return;
            }
            std::string updateColumn = trim(updateClause.substr(0, eqPos));
            std::string newValue = trim(updateClause.substr(eqPos + 1));
            // Parse whereClause if provided
            std::string conditionColumn, conditionValue;
            if (!whereClause.empty()) {
                size_t whereEqPos = whereClause.find('=');
                if (whereEqPos == std::string::npos) {
                    std::cerr << "Invalid WHERE clause format. Expected column=value\n";
                    return;
                }
                conditionColumn = trim(whereClause.substr(0, whereEqPos));
                conditionValue = trim(whereClause.substr(whereEqPos + 1));
            }
            // Open the table file
            Table table(db.getName(), tableName, {});
            std::ifstream file(table.getFilePath());
            if (!file.is_open()) {
                std::cerr << "Failed to open table file for UPDATE: " << table.getFilePath() << "\n";
                return;
            }
            std::string headerLine;
            if (!std::getline(file, headerLine)) {
                std::cerr << "Table is empty. Cannot perform UPDATE.\n";
                return;
            }
            // Parse header row
            Row headerRow(headerLine);
            auto header = headerRow.values;
            // Find indices for update and condition columns
            int updateIndex = -1;
            int conditionIndex = -1;
            for (size_t i = 0; i < header.size(); i++) {
                if (header[i] == updateColumn)
                    updateIndex = i;
                if (!conditionColumn.empty() && header[i] == conditionColumn)
                    conditionIndex = i;
            }
            if (updateIndex == -1) {
                std::cerr << "Update column '" << updateColumn << "' not found in table header.\n";
                return;
            }
            if (!conditionColumn.empty() && conditionIndex == -1) {
                std::cerr << "Condition column '" << conditionColumn << "' not found in table header.\n";
                return;
            }
            // Read all rows
            std::vector<Row> rows;
            std::string line;
            while (std::getline(file, line)) {
                rows.push_back(Row(line));
            }
            file.close();
            // Update rows based on condition (if provided) or all rows otherwise.
            for (auto& row : rows) {
                if (row.values.size() <= static_cast<size_t>(updateIndex))
                    continue; // skip malformed rows
                if (!conditionColumn.empty()) {
                    if (row.values.size() <= static_cast<size_t>(conditionIndex))
                        continue;
                    if (row.values[conditionIndex] == conditionValue) {
                        row.updateValue(updateIndex, newValue);
                    }
                } else {
                    row.updateValue(updateIndex, newValue);
                }
            }
            // Write back the header and rows.
            std::ofstream outFile(table.getFilePath(), std::ios::trunc);
            if (!outFile.is_open()) {
                std::cerr << "Failed to open table file for writing UPDATE: " << table.getFilePath() << "\n";
                return;
            }
            outFile << headerRow.toCSV() << "\n";
            for (const auto& row : rows) {
                outFile << row.toCSV() << "\n";
            }
            std::cout << "Table updated: " << tableName << "\n";
        }
        else if (action == "DELETE") {
            std::string tableName;
            if (!(iss >> tableName)) {
                std::cerr << "Missing table name in DELETE command.\n";
                return;
            }
            tableName = trim(tableName);
            std::string whereClause;
            std::getline(iss, whereClause);
            whereClause = trim(whereClause);
            std::string conditionColumn, conditionValue;
            bool conditionProvided = false;
            if (!whereClause.empty()) {
                // Expecting "WHERE column=value"
                if (whereClause.find("WHERE") == 0)
                    whereClause = trim(whereClause.substr(5));
                size_t eqPos = whereClause.find('=');
                if (eqPos == std::string::npos) {
                    std::cerr << "Invalid WHERE clause format in DELETE. Expected column=value\n";
                    return;
                }
                conditionColumn = trim(whereClause.substr(0, eqPos));
                conditionValue = trim(whereClause.substr(eqPos + 1));
                conditionProvided = true;
            }
            Table table(db.getName(), tableName, {});
            std::ifstream file(table.getFilePath());
            if (!file.is_open()) {
                std::cerr << "Failed to open table file for DELETE: " << table.getFilePath() << "\n";
                return;
            }
            std::string headerLine;
            if (!std::getline(file, headerLine)) {
                std::cerr << "Table is empty. Cannot perform DELETE.\n";
                return;
            }
            Row headerRow(headerLine);
            auto header = headerRow.values;
            int conditionIndex = -1;
            if (conditionProvided) {
                for (size_t i = 0; i < header.size(); i++) {
                    if (header[i] == conditionColumn) {
                        conditionIndex = i;
                        break;
                    }
                }
                if (conditionIndex == -1) {
                    std::cerr << "Condition column '" << conditionColumn << "' not found in table header.\n";
                    return;
                }
            }
            std::vector<Row> rows;
            std::string line;
            while (std::getline(file, line)) {
                rows.push_back(Row(line));
            }
            file.close();
            // If a condition is provided, remove rows matching condition.
            // Otherwise, delete all rows (keeping the header).
            if (conditionProvided) {
                rows.erase(std::remove_if(rows.begin(), rows.end(), [conditionIndex, conditionValue](const Row& row) {
                    return row.values.size() > static_cast<size_t>(conditionIndex) && row.values[conditionIndex] == conditionValue;
                }), rows.end());
            } else {
                // Delete all rows by clearing the vector.
                rows.clear();
            }
            std::ofstream outFile(table.getFilePath(), std::ios::trunc);
            if (!outFile.is_open()) {
                std::cerr << "Failed to open table file for writing DELETE: " << table.getFilePath() << "\n";
                return;
            }
            outFile << headerRow.toCSV() << "\n";
            for (const auto& row : rows) {
                outFile << row.toCSV() << "\n";
            }
            std::cout << "Rows deleted from table: " << tableName << "\n";
        }
        else if (action == "DROP") {
            std::string type;
            iss >> type;
            type = trim(type);
            if (type == "TABLE") {
                std::string tableName;
                if (!(iss >> tableName)) {
                    std::cerr << "Missing table name in DROP TABLE command.\n";
                    return;
                }
                tableName = trim(tableName);
                Table table(db.getName(), tableName, {});
                fs::path fileToDelete(table.getFilePath());
                if (fs::exists(fileToDelete)) {
                    try {
                        fs::remove(fileToDelete);
                        std::cout << "Table dropped: " << tableName << "\n";
                    } catch (const fs::filesystem_error &e) {
                        std::cerr << "Error dropping table: " << e.what() << "\n";
                    }
                } else {
                    std::cerr << "Table not found: " << tableName << "\n";
                }
            }
            else {
                std::cerr << "Unsupported DROP command: " << command << "\n";
            }
        }
        else {
            std::cerr << "Invalid command: " << command << "\n";
        }
    }
};

// Main DBMS class
class DBMS {
private:
    std::unordered_map<std::string, Database> databases;
public:
    void execute(const std::string& command) {
        std::istringstream iss(command);
        std::string action;
        iss >> action;
        action = trim(action);

        if (action == "CREATE") {
            std::string type;
            iss >> type;
            type = trim(type);
            if (type == "DATABASE") {
                std::string name;
                if (!(iss >> name)) {
                    std::cerr << "Missing database name in CREATE DATABASE command.\n";
                    return;
                }
                name = trim(name);
                Database db(name);
                db.create();
                databases[name] = db;
            }
            else if (type == "TABLE") {
                std::cerr << "Please use the 'USE' command to select a database before creating a table.\n";
            }
            else {
                std::cerr << "Unknown CREATE command: " << command << "\n";
            }
        } else if (action == "USE") {
            std::string dbName;
            if (!(iss >> dbName)) {
                std::cerr << "Missing database name in USE command.\n";
                return;
            }
            dbName = trim(dbName);
            // Check if the database exists in our map.
            if (databases.find(dbName) == databases.end()) {
                // If not in the map, check the file system.
                fs::path dbPath = fs::path(getDesktopDBMSPath()) / dbName;
                if (fs::exists(dbPath) && fs::is_directory(dbPath)) {
                    // Create a new Database instance and add it to our map.
                    Database db(dbName);
                    // You might also want to do some validation or initialization here.
                    databases[dbName] = db;
                } else {
                    std::cerr << "Database does not exist: " << dbName << "\n";
                    return;
                }
            }
            // Proceed with using the database...
            Parser parser;
            std::cout << "Using database: " << dbName << "\n";
            std::string nextCommand;
            while (true) {
                std::cout << dbName << "> ";
                std::getline(std::cin, nextCommand);
                nextCommand = trim(nextCommand);
                if (nextCommand == "EXIT") {
                    std::cout << "Exiting database: " << dbName << "\n";
                    break;
                }
                if (!nextCommand.empty()) {
                    parser.parseCommand(nextCommand, databases[dbName]);
                }
            }
        } else {
            std::cerr << "Unknown command: " << command << "\n";
        }
    }

    void start() {
        std::string command;
        std::cout << "Welcome to the robust DBMS! Type your commands below.\n";
        while (true) {
            std::cout << "dbms> ";
            std::getline(std::cin, command);
            command = trim(command);
            if (command == "EXIT") {
                std::cout << "Exiting DBMS. Goodbye!\n";
                break;
            }
            if (!command.empty()) {
                execute(command);
            }
        }
    }
};

int main() {
    DBMS dbms;
    dbms.start();
    return 0;
}

#include "library.cpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

string fs_path = "";
bool exitProgram = false;

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

    friend void printData();

    friend void writeData(string FILENAME_);
    friend void delete_row(const string& columnName, const string& searchData);
};

int columnWidth = 15;

unordered_map<string, Row> dataMap;
vector<string> headers;

void retriveData(string FILENAME_)
{
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

        while (getline(ss, value, ','))
        {
            rowValues.push_back(value);
        }

        if (isHeader)
        {
            headers = rowValues;
            isHeader = false;
        }
        else
        {
            dataMap[rowValues[0]] = Row(rowValues[0], vector<string>(rowValues.begin() + 1, rowValues.end()));
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

    // Print headers
    for (const auto &header : headers)
    {
        cout << setw(columnWidth) << left << header;
    }
    cout << "\n"
         << string(headers.size() * columnWidth, '-') << "\n";

    // Print data rows
    for (const auto &[id, row] : dataMap)
    {
        for (const auto &value : row.values)
        {
            cout << setw(columnWidth) << left << value;
        }
        cout << "\n";
    }
}

bool init_database(string name)
{
    if (!fs::exists(name))
    {
        if (fs::create_directory(name))
        {
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

void use(string name, string type)
{
    if (type == "dir")
    {
        fs::current_path(name);
        std::cout << "Current working directory: " << fs::current_path() << endl;
    }
    else
    {
        retriveData(name + ".csv");
    }
    return;
}

void move_up()
{
    fs::current_path("../");
    cout << "Current path" << fs_path;
    fs::path currentPath = fs::current_path();

    if (currentPath.string() == fs_path)
    {
        exitProgram = true;
    }
    cout << "Current working directory: " << fs::current_path() << endl;
    return;
}

void clean_table()
{
    dataMap.clear();
    headers.clear();
    cout << "Table cleaned.\n";
}

void writeData(string FILENAME_) {
    ofstream file(FILENAME_ + ".csv");
    if (!file.is_open()) {
        cout << "Error opening file for writing!\n";
        return;
    }
    
    // Write header row
    for (size_t i = 0; i < headers.size(); i++) {
        file << headers[i];
        if (i != headers.size() - 1) {
            file << ",";
        }
    }
    file << "\n";
    
    // Write each row from the hashmap
    for (const auto& [key, row] : dataMap) {
        // Write the id (first column) then all other values
        file << row.id;
        for (const auto& value : row.values) {
            file << "," << value;
        }
        file << "\n";
    }
    
    file.close();
    cout << "Data written to CSV file.\n";
}

#include <algorithm> // For std::find

void delete_row(const string& columnName, const string& searchData) {
    // Check if headers are available.
    if (headers.empty()) {
        cout << "Headers are not loaded." << endl;
        return;
    }

    cout << "Deleting row with " << columnName << " = \"" << searchData << "\"." << endl;
    
    // Find the index of the provided column name.
    auto headerIt = find(headers.begin(), headers.end(), columnName);
    if (headerIt == headers.end()) {
        cout << "Column \"" << columnName << "\" not found." << endl;
        return;
    }
    size_t colIndex = distance(headers.begin(), headerIt);

    // Iterate over the hashmap to find the row.
    for (auto it = dataMap.begin(); it != dataMap.end(); ++it) {
        // If the column is the first column (id), check the row's id.
        if (colIndex == 0) {
            if (it->second.id == searchData) {
                dataMap.erase(it);
                cout << "Row with " << columnName << " = \"" << searchData << "\" deleted." << endl;
                return;
            }
        } else {
            // For other columns, adjust the index since row.values doesn't include the id.
            size_t valueIndex = colIndex - 1;
            if (valueIndex < it->second.values.size() && it->second.values[valueIndex] == searchData) {
                dataMap.erase(it);
                cout << "Row with " << columnName << " = \"" << searchData << "\" deleted." << endl;
                return;
            }
        }
    }
    
    cout << "No row found with " << columnName << " = \"" << searchData << "\"." << endl;
}

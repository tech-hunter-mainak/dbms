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

class Row {
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
};

int columnWidth = 15;

unordered_map<string, Row> dataMap;
vector<string> headers;

void retriveData(string FILENAME_) {
    ifstream file(FILENAME_);
    if (!file.is_open()) {
        cout << "Error opening file!\n";
        return;
    }

    string line;
    bool isHeader = true;
    while (getline(file, line)) {
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
            dataMap[rowValues[0]] = Row(rowValues[0], vector<string>(rowValues.begin() + 1, rowValues.end()));
        }
    }
    file.close();
    cout << "Data loaded into memory.\n";
}

void printData() {
    if (headers.empty()) {
        cout << "No data to display.\n";
        return;
    }

    // Print headers
    for (const auto& header : headers) {
        cout << setw(columnWidth) << left << header;
    }
    cout << "\n" << string(headers.size() * columnWidth, '-') << "\n";

    // Print data rows
    for (const auto& [id, row] : dataMap) {
        for (const auto& value : row.values) {
            cout << setw(columnWidth) << left << value;
        }
        cout << "\n";
    }
}

bool init_database(string name){
    if (!fs::exists(name)) {
        if (fs::create_directory(name)) {
            cout << "Directory created: " << name << endl;
        } else {
            cerr << "Failed to create directory!" << endl;
            return false;
        }
    } else {
        cout << "Directory already exists!" << endl;
        return false;
    }
    return true;
}

void use(string name, string type){
    if(type == "dir"){
        fs::current_path(name);
        std::cout << "Current working directory: " << fs::current_path() << endl;
    } else {
        retriveData(name + ".csv");
    }
    return ;
}



void move_up(){
    fs::current_path("../");
    cout<<"Current path"<<fs_path;
    fs::path currentPath = fs::current_path();

    if(currentPath.string() == fs_path){
        exitProgram = true;
    }
    cout << "Current working directory: " << fs::current_path() << endl;
    return ;
}

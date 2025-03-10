#include "library.cpp"
string fs_path = "";
bool exitProgram = false;

struct Row {
    string id;
    vector<string> values;
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
            string id = rowValues[0]; // Assume first column is unique ID
            dataMap[id] = {id, rowValues};
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

    // Set column width

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

// Funtion to make a database
bool init_database(string name){
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
    std::cout << "Current working directory: " << fs::current_path() << endl;
    return ;
}
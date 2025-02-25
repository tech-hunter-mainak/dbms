#include "library.cpp"

struct Row {
    string id;
    vector<string> values;
};

int columnWidth = 15;
const auto FILENAME_ = "./data.csv";
unordered_map<string, Row> dataMap;
vector<string> headers;

void retriveData() {
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
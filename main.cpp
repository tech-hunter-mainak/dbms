// main.cpp
#include "library.cpp"
#include "parser.cpp"
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char const *argv[])
{
    // Welcome message
    cout << "Welcome to the robust DBMS! Type your commands below." << endl;

    // Initialize DBMS parent directory "DBMS"
    string dbmsFolder = "DBMS";
    if (!fs::exists(dbmsFolder))
    {
        if (fs::create_directory(dbmsFolder))
        {
            cout << "DBMS folder created at: \"" << fs::absolute(dbmsFolder).string() << "\"" << endl;
        }
        else
        {
            cerr << "Failed to create DBMS folder!" << endl;
            return 1;
        }
    }
    else
    {
        cout << "DBMS folder already exists at: \"" << fs::absolute(dbmsFolder).string() << "\"" << endl;
    }
    fs::current_path(dbmsFolder);
    fs_path = fs::current_path().string();
    // fs_dbms_path = "../" + fs_path;

    while (!exitProgram)
    {
        // Determine prompt based on context:
        // - "dbms>" when no database is in use,
        // - "<database>>" when a database is selected,
        // - or "<table>>" if a table is active.
        string prompt;
        if (currentDatabase.empty())
            prompt = "dbms> ";
        else if (!currentDatabase.empty() && currentTable.empty())
            prompt = currentDatabase + "> ";
        else if (!currentTable.empty())
            prompt = currentTable + "> ";
        
        cout << prompt;
        list<string> query = input();
        if (!query.empty())
        {
            parser parse(query);
        }
    }
    return 0;
}

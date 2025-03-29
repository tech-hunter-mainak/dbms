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
    fs::current_path(dbmsFolder);
    fs_path = fs::current_path().string();
    // fs_dbms_path = "../" + fs_path;

    while (!exitProgram)
    {
        string prompt;
        if (currentDatabase.empty())
            prompt = "dbms> ";
        else if (!currentDatabase.empty() && currentTable.empty())
            prompt = currentDatabase + "> ";
        else if (!currentTable.empty())
            prompt = currentTable + "> ";
        
        cout << prompt;
        list<string> tokens = input();
        if (!tokens.empty())
        {
            // Split tokens by pipe ("|") symbol.
            list<list<string>> queries = splitQueries(tokens);
            // Process each query group.
            for (auto &q : queries) {
                // You can do additional trimming or check here if needed.
                parser p(q);
            }
        }
    }
    return 0;
}

#include "parser.cpp"

using namespace std;
namespace fs = std::filesystem;

bool exitProgram = false;
string fs_path = "DBMS";    // or whatever the root directory should be
string currentDatabase = "";
string currentTable = "";

int main(int argc, char const *argv[])
{
    cout << "Welcome to the robust DBMS! Type your commands below." << endl;
    string dbmsFolder = "DBMS";
    if (!fs::exists(dbmsFolder))
    {
        if (!fs::create_directory(dbmsFolder))
        {
            cerr << "Failed to configure Database, go through docs" << endl;
            return 1;
        }
    }
    // This changes the working directory to the root
    fs::current_path(dbmsFolder);
    // This updates the global variable to access in different parsing operations.
    fs_path = fs::current_path().string();
    while (!exitProgram)
    {
        string prompt;
        // You can adjust these variables to desired prompts
        if (currentDatabase.empty())
            prompt = "dbms >> ";
        else if (!currentDatabase.empty() && currentTable.empty())
            prompt = currentDatabase + " >> ";
        else if (!currentTable.empty())
            prompt = currentTable + " >> ";
        cout << prompt;
        list<string> tokens = input();
        if (!tokens.empty())
        {
            // Split tokens by pipe ("|") symbol.
            list<list<string>> queries = splitQueries(tokens);
            for (auto &q : queries) {
                Parser parser(q);
                parser.parse();
            }
        }
    }
    return 0;
}

#include "parser.cpp"

using namespace std;
namespace fs = std::filesystem;

bool exitProgram = false;
#include <cstdlib>
#include <string>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#elif __APPLE__ || __linux__
    #include <unistd.h>
    #include <pwd.h>
#endif

std::string getHomeDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path);
    }
    return "";  // fallback
#else
    const char* homedir;
    if ((homedir = getenv("HOME")) == nullptr) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    return std::string(homedir);
#endif
}

std::string getDBMSPath() {
    std::string home = getHomeDirectory();
    std::string dbmsPath = home + "/Desktop/DBMS";
    return dbmsPath;
}

// Global variable
std::string fs_path = getDBMSPath();   // or whatever the root directory should be
string currentDatabase = "";
string currentTable = "";
int main(int argc, char const *argv[])
{
    cout << "\033[1;35mWelcome to the MiniDB! Type your commands below.\033[0m" << endl;
    // string dbmsFolder = "DBMS"; // ----------->>>>>>> set the required absolute path
    if (!fs::exists(fs_path))
    {
        if (!fs::create_directory(fs_path))
        {
            cerr << "Failed to configure Database, go through docs" << endl;
            return 1;
        }
    }
    // This changes the working directory to the root
    fs::current_path(fs_path);
    // // This updates the global variable to access in different parsing operations.
    // fs_path = fs::current_path().string();
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

        cout << "\033[1;34m" << prompt << "\033[0m"; // blue bold prompt
        try{
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
        } catch (const std::invalid_argument &e) {
            cerr << "\033[31m" << e.what() << "\033[0m" << endl << endl;
            // return;
        } catch (const std::logic_error &e) {
            cerr << "\033[31m" << e.what() << "\033[0m" << endl << endl;
            // return;
        } catch (const string &msg) {
            cerr << "\033[31m" << msg << "\033[0m" << endl << endl;
            // return; 
        } catch (const char* msg) {
            cerr << "\033[31m" << msg << "\033[0m" << endl << endl;
            // return;
        } catch (...) {
            cerr << "\033[31mUnknown Error occurred while processing query.\033[0m" << endl << endl;
            // return;
        }
    }
    return 0;
}

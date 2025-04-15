#include <stdexcept>
#include <cstdlib>
#include <string>
#include <iostream>
#include <list>
#include <filesystem>

#ifdef _WIN32
#ifdef byte
#undef byte
#endif
#include <windows.h>
#include <shlobj.h>
#include <cstddef>
#else
#include <unistd.h>
#include <pwd.h>
#endif

#include "parser.cpp"

using namespace std;
namespace fs = std::filesystem;
bool exitProgram = false;
std::string aesKey = "";

std::string getHomeDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    // CSIDL_PROFILE points to the user's profile folder.
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path);
    }
    return "";  // fallback if getting profile path fails
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
    #ifdef _WIN32
        std::string dbmsPath = home + "\\QiloDB";
    #else
        std::string dbmsPath = home + "/Library/Application Support/qiloDB/data";
    #endif
    return dbmsPath;
}

bool verifyAESKey() {
    std::string inputKey;
    std::cout << "\033[1;33mEnter AES Key (32-byte password): \033[0m";
    std::getline(std::cin, inputKey);

    // Construct path to pass.txt
    std::string passFilePath = getDBMSPath(); // e.g., ".../Library/Application Support/qiloDB/data"
    passFilePath = passFilePath.substr(0, passFilePath.find_last_of("/\\")); // strip /data
    passFilePath += "/pass.txt";

    std::ifstream passFile(passFilePath);
    if (!passFile) {
        std::cerr << "\033[31mFailed to open password file at: " << passFilePath << "\033[0m" << std::endl;
        return false;
    }

    std::string storedHash;
    std::getline(passFile, storedHash);
    passFile.close();

    // Hash user input and compare
    std::string hashedInput = sha256WithSalt(inputKey, "qiloDBnits");
    cout << storedHash << endl;
    cout << hashedInput << endl;
    if (hashedInput != storedHash) {
        std::cerr << "\033[31mIncorrect AES key. Try again: \033[0m" << std::endl;
        verifyAESKey();
    }

    std:: string key = sha256WithSalt(hashedInput,"heyItsqilo");
    // Pad/truncate to 32 bytes
    if (key.length() < 32) {
        key.append(32 - key.length(), '0');
    } else if (key.length() > 32) {
        key = key.substr(0, 32);
    }
    aesKey = key;
    return true;
}

// Global variable
std::string fs_path = getDBMSPath();   // or whatever the root directory should be
std::string currentDatabase = "";
std::string currentTable = "";

int main(int argc, char const *argv[])
{
    if (!verifyAESKey()) {
        return 1;
    }
    cout << "\033[1;35mWelcome to the QiloDB! Type your commands below.\033[0m" << endl;
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

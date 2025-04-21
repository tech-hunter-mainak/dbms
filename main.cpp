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

// This is to mitigate any problems with ascii escape codes
#include <csignal>

#ifdef _WIN32
#include <windows.h>
BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT) {
        std::cin.clear();
        // Print the reset code
        std::cout << "\033[0m";
        // Clear cin’s state
        
        // Return TRUE to say “I handled it, don’t kill us”
        return FALSE;
    }
    return FALSE;  // other signals → default
}
#else
void sigintHandler(int /*signum*/) {
    // Clear std::cin’s error (so getline() can be called again)
    std::cin.clear();
    // Print the ANSI reset
    std::cout << "\033[0m";
    // Re‑install ourselves for next time
    std::exit(0);
    // Do *not* re‑raise or exit
}
#endif
 
static constexpr char const* APP_NAME    = "qiloDB";
static constexpr char const* APP_VERSION = "1.0.0";
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
    #ifdef _WIN32
        std::string dbmsPath = home + "\\QiloDB\\data";
    #else 
        std::string dbmsPath = home + "/Library/Application Support/qiloDB/data";
    #endif
    return dbmsPath;
}
// Global variable
string fs_path = getDBMSPath();   // or whatever the root directory should be
string currentDatabase = "";
string currentTable = "";
std::string aesKey = "";
Table* currentTableInstance = nullptr;
bool exitProgram = false;
int incorrectAttempts = 0;
// Derive 256-bit AES key from raw password
std::string deriveAESKey(const std::string& hashedInput) {
    std::string key = sha256WithSalt(hashedInput, "heyItsqilo");
    if (key.length() < 32)
        key.append(32 - key.length(), '0');
    else if (key.length() > 32)
        key = key.substr(0, 32);
    return key;
}

void rotateEncryption(const std::string& oldKey, const std::string& newKey) {
    for (auto& dbEntry : fs::directory_iterator(fs_path)) {
        if (!dbEntry.is_directory()) continue;

        for (auto& fileEntry : fs::directory_iterator(dbEntry.path())) {
            if (!fileEntry.is_regular_file()) continue;
            if (fileEntry.path().extension() != ".bin")   // <<---- only rotate your encrypted tables
                continue;

            // Read IV + ciphertext
            std::ifstream in(fileEntry.path(), std::ios::binary);
            std::string iv(AES_BLOCK_SIZE, '\0');
            in.read(&iv[0], AES_BLOCK_SIZE);
            std::string cipherText{ std::istreambuf_iterator<char>(in),
                                     std::istreambuf_iterator<char>() };
            in.close();

            // Decrypt & re‑encrypt under try/catch so one bad file won’t abort everything
            try {
                std::string savedKey = aesKey;

                aesKey = oldKey;
                std::string plain = aesDecrypt(cipherText, iv);
                aesKey = newKey;
                std::string newIv;
                std::string newCipher = aesEncrypt(plain, newIv);

                std::ofstream out(fileEntry.path(), std::ios::binary | std::ios::trunc);
                out.write(newIv.data(), newIv.size());
                out.write(newCipher.data(), newCipher.size());
                out.close();

                aesKey = savedKey;
            } 
            catch (const std::exception &e) {
                std::cerr << "Warning: could not rotate "
                          << fileEntry.path().filename().string()
                          << " — " << e.what() << "\n";
            }
        }
    }
    // finally switch your global key so new sessions use the new one
    // aesKey = newKey;
}
void checkAttempts() {
    std::cout << "Remaining Attempts left: " << (3 - incorrectAttempts) << std::endl;

    if (incorrectAttempts == 2) {
        std::cerr << "\033[1mWarning: Data will be deleted on the next incorrect attempt.\033[0m" 
                  << std::endl;
        cout << "\033[1mCheck docs to know how to reset the password. Or use --help\033[0m" << endl;
    }
    else if (incorrectAttempts >= 3) {
        std::cerr << "\033[1;31mToo many incorrect attempts. Deleting all data...\033[0m" 
                  << std::endl;
        try {
            // Remove every file and subdirectory under fs_path
            for (auto& entry : fs::directory_iterator(fs_path)) {
                fs::remove_all(entry.path());
            }
            std::cerr << "All data deleted." <<endl;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error deleting data: " << e.what() << std::endl;
        }
        exit(0);
    }
}

bool verifyAESKey() {
    std::string inputKey;
    std::cout << "\033[1;33mEnter password: \033[0m";
    std::getline(std::cin, inputKey);

    // std::string passFilePath = getDBMSPath(); // e.g., ".../Library/Application Support/qiloDB/data" or "...\\QiloDB\\data"
    fs::path passFilePath = fs::path(getDBMSPath()).parent_path() / "pass.txt";
    // cout << "Password File Path" << passFilePath <<endl;
    std::ifstream passFile(passFilePath.c_str());
    if (!passFile) {
        std::cerr << "\033[31mFailed to retrive password.\033[0m" << std::endl;
        return false;
    }

    std::string storedHash;
    std::getline(passFile, storedHash);
    passFile.close();

    // Hash user input and compare
    std::string hashedInput = sha256WithSalt(inputKey, "qiloDBnits");
    
    if (hashedInput != storedHash) {
        std::cerr << "\033[31mIncorrect AES key. Try again: \033[0m" << std::endl;
        incorrectAttempts += 1;
        checkAttempts();
        verifyAESKey();
    }
    aesKey = deriveAESKey(hashedInput);
    return true;
}
bool passwordForgot(){
    // std::string passFilePath = getDBMSPath(); // e.g., ".../Library/Application Support/qiloDB/data" or "...\\QiloDB\\data"
    fs::path passFilePath = fs::path(getDBMSPath()).parent_path() / "pass.txt";
    // cout << "Password File Path" << passFilePath <<endl;
    std::ifstream passIn(passFilePath.c_str());
    if (!passIn) {
        std::cerr << "\033[31mFailed to retrive password.\033[0m" << std::endl;
        return false;
    }

    std::string storedHash;
    std::getline(passIn, storedHash);
    passIn.close();
    std::string oldAESKey = deriveAESKey(storedHash);

    // 2) Prompt for new password (with confirmation)
    std::string newPass, confirm;
    do {
        std::cout << "Enter new password: ";
        std::getline(std::cin, newPass);
        std::cout << "Confirm new password: ";
        std::getline(std::cin, confirm);
        if (newPass != confirm)
            std::cout << "\033[31mPasswords do not match. Try again.\033[0m\n";
    } while (newPass != confirm);
    // 3) Compute the new pass‑file hash & derive the new AES key
    std::string newPassHash = sha256WithSalt(newPass, "qiloDBnits");
    std::string newAESKey   = deriveAESKey(newPassHash);

    // 4) Rotate every .bin under each database
    rotateEncryption(oldAESKey, newAESKey);
    // 5) Overwrite pass.txt with the new hash
    fs::path passFile = fs::path(getDBMSPath()).parent_path() / "pass.txt";
    std::ofstream out(passFile, std::ios::trunc);
    out << newPassHash;
    out.close();

    // 6) Switch your running key to the new one
    aesKey = newAESKey;
    std::cout << "\033[32mPassword updated successfully.\033[0m\n";
    return true;
}
int main(int argc, char const *argv[])
{   
    #ifdef _WIN32
        SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
    #else
        std::signal(SIGINT, sigintHandler);
    #endif
    // --- Command-line flag handling ---
    if (argc > 1) {
        std::string flag{argv[1]};
        if (flag == "--version" || flag == "-v") {
            std::cout << APP_NAME << " " << APP_VERSION << std::endl;
            return 0;
        } else if (flag == "--help" || flag == "-h") {
            processHelp();
            return 0;
        } else if (flag == "--loc") {
            std::cout << "Database path: " << fs_path << std::endl;
            return 0;
        } else if (flag == "--forgot") {
            if(!passwordForgot()) return 1;
            else return 0;
        }        
    }
    // --- End flag handling ---
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
        } catch (const std::runtime_error &e) {
            cerr << "\033[31m" << e.what() << "\033[0m" << endl << endl;
        } catch (const std::invalid_argument &e) {
            cerr << "\033[31m" << e.what() << "\033[0m" << endl << endl;
        } catch (const std::logic_error &e) {
            cerr << "\033[31m" << e.what() << "\033[0m" << endl << endl;
        } catch (const string &msg) {
            cerr << "\033[31m" << msg << "\033[0m" << endl << endl;
        } catch (const char* msg) {
            cerr << "\033[31m" << msg << "\033[0m" << endl << endl;
        } catch (...) {
            cerr << "\033[31mUnknown Error occurred while processing query.\033[0m" << endl << endl;
        }
    }
    return 0;
}
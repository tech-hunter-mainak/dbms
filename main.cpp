#include "parser.cpp"
#include <readline/readline.h>
#include <readline/history.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

bool exitProgram = false;
string fs_path = "DBMS";
string currentDatabase = "";
string currentTable = "";

// --- Custom Keywords for Auto-completion ---
vector<string> keywords = {
    "init", "erase", "empty", "clean", "make", "del", "change", "insert", "enter", "choose",
    "exit", "or", "and", "where", "like", "show", "rollback", "commit", "list", "close",
    "head", "limit", "to", "describe"
};

vector<string> completions;

// Function to get .csv files (only stem, no extension)
vector<string> getCSVStemsInCurrentPath() {
    vector<string> stems;
    for (const auto& entry : fs::directory_iterator(fs::current_path())) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            stems.push_back(entry.path().stem().string());
        }
    }
    return stems;
}

char* command_generator(const char* text, int state) {
    static size_t index;
    static string prefix;

    if (state == 0) {
        completions.clear();
        prefix = text;

        vector<string> csvStems = getCSVStemsInCurrentPath();
        for (const auto& kw : keywords) {
            if (kw.rfind(prefix, 0) == 0) completions.push_back(kw);
        }
        for (const auto& stem : csvStems) {
            if (stem.rfind(prefix, 0) == 0) completions.push_back(stem);
        }

        if (completions.size() > 1) {
            cout << "\n\033[1;33m"; // Yellow bold
            for (const auto& word : completions) cout << word << "  ";
            cout << "\033[0m" << endl;
            rl_on_new_line();
            rl_redisplay();
        }

        index = 0;
    }

    if (index < completions.size()) {
        return strdup(completions[index++].c_str());
    }
    return nullptr;
}

char** custom_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_generator);
}

int main(int argc, char const *argv[]) {
    // Cross-platform readline compatibility handled externally
    rl_attempted_completion_function = custom_completion;
    read_history(".dbms_history");

    cout << "\033[1;32mWelcome to the robust DBMS! Type your commands below.\033[0m" << endl;

    string dbmsFolder = "DBMS";
    if (!fs::exists(dbmsFolder)) {
        if (!fs::create_directory(dbmsFolder)) {
            cerr << "\033[31mFailed to configure Database, go through docs\033[0m" << endl;
            return 1;
        }
    }

    fs::current_path(dbmsFolder);
    fs_path = fs::current_path().string();

    while (!exitProgram) {
        string prompt;
        if (currentDatabase.empty())
            prompt = "dbms >> ";
        else if (!currentDatabase.empty() && currentTable.empty())
            prompt = currentDatabase + " >> ";
        else
            prompt = currentTable + " >> ";

        string coloredPrompt = "\001\033[1;34m\002" + prompt + "\001\033[0m\002";
        char* input_cstr = readline(coloredPrompt.c_str());

        if (!input_cstr) continue;

        string input_str = input_cstr;
        free(input_cstr);

        if (input_str.empty()) continue;

        add_history(input_str.c_str());

        try {
            list<string> tokens = input(input_str);
            if (!tokens.empty()) {
                list<list<string>> queries = splitQueries(tokens);
                for (auto &q : queries) {
                    Parser parser(q);
                    parser.parse();
                }
            }
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

    write_history(".dbms_history");
    return 0;
}

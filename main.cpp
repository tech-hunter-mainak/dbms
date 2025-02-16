#include "library.cpp"
#include "parser.cpp"

int main(int argc, char const *argv[])
{
    std::string dirName = "database";

    // Create directory if it does not exist
    if (!fs::exists(dirName))
    {
        if (fs::create_directory(dirName))
        {
            cout << "Directory created: " << dirName << endl;
        }
        else
        {
            cerr << "Failed to create directory!" << endl;
            return 1;
        }
    }
    else
    {
        cout << "Directory already exists!" << endl;
    }

    parser parse("CREATE DATABASE myDB");
    for (auto i : parse.split())
    {
        cout << i << " ";
    }

    // Change the current working directory to the new directory
    fs::current_path(dirName);
    std::cout << "Current working directory: " << fs::current_path() << endl;

    menu();

    return 0;
}

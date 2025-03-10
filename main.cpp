#include "library.cpp"
#include "parser.cpp"

int main(int argc, char const *argv[])
{
    // Initialises database Parent folder 
    std::string dirName = "DBMS";
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
    fs::current_path(dirName);

    fs::path filePath = fs::current_path();
    fs_path = filePath.string();
    std::cout << "Current working directory: " << fs::current_path() << endl;
    
    // menu();
    while(true){
        list<string> query = input();
        parser parse(query);
        retriveData();
        printData();
    }
    return 0;
}

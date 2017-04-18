#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include "boost/filesystem.hpp"

using namespace std;
namespace fs = boost::filesystem;

class BaseRenamer {
    public:
        /* Constructor */
        BaseRenamer();
        /* Rename files with the appropriate directory prefix */
        void dir_rename(string old, string n);
        /* Lists the items in the directory */
        vector<string> & listdir();
        /* Filters the files by a regex pattern */
        vector<string> & filterfiles(regex pattern);
        /* Insert and shift the names in the list, simultaneously renaming the files */
        void shiftnames(int origpos, int newpos);
    protected:
        /* List of files */
        vector<string> files;
};

class CLIRenamer : public BaseRenamer {
    public:
        /* Constructor */
        CLIRenamer();
        /* CLI commands */
        void InterpretCommands();
        void InterpretChangeDir(stringstream & line);
        void InterpretList(stringstream & line);
        void InterpretSwitch(stringstream & line);
        void InterpretQuit();
        void InterpretHelp();
};

class GUIRenamer : public BaseRenamer {
    public:
        /* Constructor */
        GUIRenamer();
};

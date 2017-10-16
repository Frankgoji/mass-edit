#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "boost/filesystem.hpp"

using namespace std;
namespace fs = boost::filesystem;

/* Used for indicating a range of numbers. The range is [l, u). (end-exclusive) */
class Range {
    public:
        Range(int begin, int end);
        Range(string s);
        static bool IsRange(string s);
        bool OutOfRange(int n);
        bool OutOfRange(Range r);
        int Next(int n);
        int Prev(int n);
        int Span();     // upper - lower
        int begin();
        int end();
        Range reverse();
    private:
        int start;
        int last;
};
ostream & operator<< (ostream & os, Range r);

class BaseRenamer {
    public:
        /* Constructor */
        BaseRenamer();
        /* Rename files with the appropriate directory prefix */
        void dir_rename(string old, string n);
        /* Lists the items in the directory */
        vector<string> & listdir();
        /* Normalize filename lengths */
        void normalize(int numZeros);
        /* Normalize this filename lengths up to numZeros */
        string normalize(string filename, int numZeros);
        /* Filters the files by a regex pattern */
        vector<string> & filterfiles(regex pattern);
        /* Insert and shift the names in the list, simultaneously renaming the files */
        void insert(Range origpositions, int newpos);
        /* Adds certain range of names by a number */
        void shiftnames(Range files, int add);
    protected:
        /* List of files */
        vector<string> files;
        /* Longest file name */
        size_t longestName;
        /* Adds amt to the file name number */
        string addAmt(string filename, int amt);
        /* Checks if a shift will cause any file collisions */
        bool check_shift(Range fileRange, int shift);
};

class CLIRenamer : public BaseRenamer {
    public:
        /* Constructor */
        CLIRenamer();
        /* CLI commands */
        void InterpretCommands();
        void InterpretChangeDir(stringstream & line);
        void InterpretList(stringstream & line);
        void InterpretShift(stringstream & line);
        void InterpretInsert(stringstream & line);
        void InterpretQuit();
        void InterpretHelp(string errmessage);
};

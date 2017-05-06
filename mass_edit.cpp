#include "mass_edit.h"

using namespace std;

/* Range class */
Range::Range(int b, int e)
    : start(b),
      last(e)
{}
Range::Range(string s) {
    if (IsRange(s)) {
        stringstream sm(s);
        sm >> start;
        sm.get();
        sm >> last;
    } else {
        throw logic_error("Not a valid string");
    }
}
int Range::begin() { return start; }
int Range::end() { return last; }
bool Range::IsRange(string s) {
    stringstream sm(s);
    int first, last;
    return (sm >> first) && (sm.get() == '-') && (sm >> last);
}
bool Range::OutOfRange(int n) {
    return (start < last && (n >= last || n < start))
        || (start > last && (n <= last || n > start));
}
int Range::Next(int n) {
    if (start <= last) {
        return n+1;
    }
    return n-1;
}
int Range::Span() {
    return abs(last - start);
}
Range Range::reverse() {
    return Range(last-1, start-1);
}

/* BaseRenamer: Constructor */
BaseRenamer::BaseRenamer()
    : files(0)
{
    listdir();
}

/* Rename files with the appropriate directory prefix */
void BaseRenamer::dir_rename(string old, string n) {
    fs::rename(fs::current_path() / old, fs::current_path() / n);
}

/* Lists the items in the directory */
vector<string> & BaseRenamer::listdir() {
    if (files.size() != 0) {
        files = vector<string>(0);
    }
    fs::directory_iterator enditr;
    for (fs::directory_iterator diritr(fs::current_path());
            diritr != enditr;
            diritr++) {
        string s(diritr->path().filename().string());
        files.push_back(s);
    }
    sort(files.begin(), files.end());
    return files;
}

/* Normalize filename lengths up to numZeros */
void BaseRenamer::normalize(int numZeros) {
    for (size_t i = 0; i < files.size(); i++) {
        stringstream name;
        name << setfill('0') << setw(numZeros) << files[i];
        dir_rename(files[i], name.str());
    }
    listdir();
}

/* Filters the files by a regex pattern */
vector<string> & BaseRenamer::filterfiles(regex pattern) {
    vector<string>::iterator b = files.begin();
    while (b != files.end()) {
        if (!regex_match(*b, pattern)) {
            b = files.erase(b);
        } else {
            b++;
        }
    }
    return files;
}

/* Insert and shift the names in the list, simultaneously renaming the files.
 * The item(s) at origpositions will be moved to newpos and everything else will
 * be shifted over.
 * Precondition: files has been populated by listdir. */
void BaseRenamer::insert(Range origpositions, int newpos) {
    if (!origpositions.OutOfRange(newpos)) {
        cerr << "Cannot insert within a range." << endl;
        return;
    }
    // Rename files in origpositions to temps
    for (int i = origpositions.begin(); !origpositions.OutOfRange(i); i = origpositions.Next(i)) {
        dir_rename(files[i], "temp" + to_string(i));
    }
    int offset;
    if (origpositions.end() <= newpos) {
        offset = newpos - origpositions.end() + 1;
        for (int i = origpositions.end(); i <= newpos; i++) {
            // Rename files[i+span] to files[i]
            dir_rename(files[i], files[i-origpositions.Span()]);
        }
    } else {
        offset = newpos - origpositions.begin();
        for (int i = origpositions.begin()-1; i >= newpos; i--) {
            // Rename files[i-span] to files[i]
            dir_rename(files[i], files[i+origpositions.Span()]);
        }
    }
    // Rename temp files to newpos
    for (int i = origpositions.begin(); !origpositions.OutOfRange(i); i = origpositions.Next(i)) {
        dir_rename("temp" + to_string(i), files[i + offset]);
    }
}

/* Adds certain range of names by a number.
 * Precondition: the range and the amount to add don't break filenames. */
void BaseRenamer::shiftnames(Range fileRange, int add) {
    size_t longestName = 0;
    for (int i = fileRange.begin(); !fileRange.OutOfRange(i); i = fileRange.Next(i)) {
        int fileNum;
        stringstream strm(files[i]);
        size_t suffixstart = files[i].find_last_of(".");
        if (strm >> fileNum) {
            strm.clear();
            fileNum += add;
            strm << fileNum;
            string suffix = "";
            if (suffixstart != string::npos) {
                suffix = files[i].substr(suffixstart);
                strm << suffix;
            }
            if (strm.str().length() > longestName) {
                longestName = strm.str().length();
            }
            dir_rename(files[i], strm.str());
        } else {
            cerr << "File doesn't start with number." << endl;
            break;
        }
    }
    listdir();
    normalize(longestName);
}

/* Constructor */
CLIRenamer::CLIRenamer()
    : BaseRenamer()
{}

/* CLI commands */
/* Interprets user input and CLI commands in a REPL loop */
void CLIRenamer::InterpretCommands() {
    string line, first;
    stringstream linestrm;
    while (true) {
        cout << fs::current_path().string() << "> ";
        getline(cin, line);
        linestrm = stringstream(line);
        if (linestrm >> first) {
            if (first == "cd") {
                InterpretChangeDir(linestrm);
            } else if (first == "ls") {
                InterpretList(linestrm);
            } else if (first == "switch") {
                InterpretInsert(linestrm);
            } else if (first == "quit") {
                InterpretQuit();
            } else {
                InterpretHelp();
            }
        }
    }
}

/* Do change directory */
void CLIRenamer::InterpretChangeDir(stringstream & line) {
    string dir;
    line >> dir;
    if (dir[0] == '~') {
        cerr << "Please don't use the '~' symbol for the home directory." << endl;
    } else {
        try {
            fs::current_path(dir);
            listdir();
        } catch (fs::filesystem_error) {
            perror("Cannot change directory");
        }
    }
}

/* Do list directory */
void CLIRenamer::InterpretList(stringstream & line) {
    vector<string> f = listdir();
    f = filterfiles(regex("\\d+(\\.[a-zA-Z]{3})?"));
    for (size_t i = 0; i < f.size(); i++) {
        cout << i << ". " << f[i] << endl;
    }
}

/* Interpret the shift command */
void InterpretShift(stringstream & line) {
    //
}

/* Interpret the insert command */
void CLIRenamer::InterpretInsert(stringstream & line) {
    int old, n;
    line >> old >> n;
    //insert(old, n);
}

/* Quit */
void CLIRenamer::InterpretQuit() {
    exit(0);
}

/* Usage */
void CLIRenamer::InterpretHelp() {
    cout << "Usage:" << endl;
    cout << left << setw(30) << "cd <directory>" << setw(40) << "change directory" << endl;
    cout << left << setw(30) << "ls" << setw(40) << "list directory contents with indices" << endl;
    cout << left << setw(30) << "shift <all|range> <amount>" << setw(40) << "shift file numbers by some amount. shift <amt> defaults to all" << endl;
    cout << left << setw(30) << "insert <range|index> <index>" << setw(40) << "switch items 1 and 2, appropriately shifting the other items" << endl;
    cout << "quit" << endl;
}

int main() {
    CLIRenamer cli;
    cli.InterpretCommands();

    return 0;
}

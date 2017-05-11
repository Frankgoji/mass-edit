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
/* Returns true if r is not entirely within the range */
bool Range::OutOfRange(Range r) {
    return (OutOfRange(r.begin()) || OutOfRange(r.Prev(r.end())));
}
int Range::Next(int n) {
    if (start <= last) {
        return n+1;
    }
    return n-1;
}
int Range::Prev(int n) {
    if (start <= last) {
        return n-1;
    }
    return n+1;
}
int Range::Span() {
    return abs(last - start);
}
Range Range::reverse() {
    return Range(Prev(last), Prev(start));
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
        offset = newpos - origpositions.end();
        for (int i = origpositions.end(); i < newpos; i++) {
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
            stringstream newFile("");
            fileNum += add;
            newFile << fileNum;
            string suffix = "";
            if (suffixstart != string::npos) {
                cout << "adding suffix?" << endl;
                suffix = files[i].substr(suffixstart);
                newFile << suffix;
            }
            if (newFile.str().length() > longestName) {
                longestName = newFile.str().length();
            }
            dir_rename(files[i], newFile.str());
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
            } else if (first == "shift") {
                InterpretShift(linestrm);
            } else if (first == "insert") {
                InterpretInsert(linestrm);
            } else if (first == "quit") {
                InterpretQuit();
            } else {
                InterpretHelp("");
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
void CLIRenamer::InterpretShift(stringstream & line) {
    string firstarg;
    int amt;
    if (!(line >> firstarg)) {
        InterpretHelp("shift command needs at least one argument");
        return;
    }
    Range * r;
    if (firstarg == "all") {    // shift all files
        r = new Range(0, files.size());
    } else if (Range::IsRange(firstarg)) {  // read range
        r = new Range(firstarg);
    } else {
        stringstream first(firstarg);   // Check if it's a number
        if (!(first >> amt)) {
            InterpretHelp("incorrect shift usage");
            return;
        }
        r = new Range(0, files.size()); // If so, shift all
    }
    // Get amount to shift by
    if ((firstarg == "all" || Range::IsRange(firstarg)) && !(line >> amt)) {
        InterpretHelp("");
        return;
    }
    // perform error checking on the input
    Range filesIndex(0, files.size());
    if (filesIndex.OutOfRange(*r)) {
        InterpretHelp("Files are out of range\n");
    } else if ((r->begin() != 0 || r->end() != filesIndex.end())
            && r->begin() + amt < filesIndex.end()) {  // not shifting all, but causes a conflict
        InterpretHelp("File collision illegal\n");
    } else {
        shiftnames(*r, amt);
    }
}

/* Interpret the insert command */
void CLIRenamer::InterpretInsert(stringstream & line) {
    string first;
    int index2;
    if (!(line >> first)) { // Check the first arg
        InterpretHelp("insert command needs at least two arguments");
        return;
    }
    Range * r;
    if (Range::IsRange(first)) {    // first arg is range
        r = new Range(first);
    } else {                        // first arg is index
        int index1;
        stringstream firststr(first);
        if (!(firststr >> index1)) {
            InterpretHelp("incorrect insert usage");
            return;
        }
        r = new Range(index1, index1 + 1);
    }
    if (!(line >> index2)) {        // second arg is index
        InterpretHelp("incorrect insert usage");
        return;
    }
    // error check, call function
    Range filesIndex(0, files.size());
    if (filesIndex.OutOfRange(*r)) {
        InterpretHelp("Files out of range\n");
    } else if (filesIndex.OutOfRange(index2) && index2 != filesIndex.end()) {
        InterpretHelp("Insert point out of range\n");
    } else if (!r->OutOfRange(index2)) {
        InterpretHelp("Cannot insert file into the same range\n");
    } else {
        insert(*r, index2);
    }
}

/* Quit */
void CLIRenamer::InterpretQuit() {
    exit(0);
}

/* Usage */
void CLIRenamer::InterpretHelp(string errmessage) {
    if (errmessage != "") {
        cerr << errmessage << endl;
    }
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

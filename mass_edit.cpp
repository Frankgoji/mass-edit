#include "mass_edit.h"

using namespace std;

/********** Range class **********/
/* Constructor */
Range::Range(int b, int e)
    : start(b),
      last(e)
{}

/* Constructor with string, e.g. num1-num2 */
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

/* Getters */
int Range::begin() { return start; }
int Range::end() { return last; }

/* Static function, determines if the string is a range that can be used in a
 * constructor */
bool Range::IsRange(string s) {
    stringstream sm(s);
    int first, last;
    return (sm >> first) && (sm.get() == '-') && (sm >> last);
}

/* Returns true if n is out of the range */
bool Range::OutOfRange(int n) {
    return (start < last && (n >= last || n < start))
        || (start > last && (n <= last || n > start));
}

/* Returns true if r is not entirely within the range */
bool Range::OutOfRange(Range r) {
    return (OutOfRange(r.begin()) || OutOfRange(r.Prev(r.end())));
}

/* Increment towards end */
int Range::Next(int n) {
    if (start <= last) {
        return n+1;
    }
    return n-1;
}

/* Increment towards begin */
int Range::Prev(int n) {
    if (start <= last) {
        return n-1;
    }
    return n+1;
}

/* Return the span of the range */
int Range::Span() {
    return abs(last - start);
}

/* Return the reverse of the range */
Range Range::reverse() {
    return Range(Prev(last), Prev(start));
}

/* Operator overload for cout */
ostream & operator<< (ostream & os, Range r) {
    os << r.begin() << "-" << r.end();
    return os;
}

/********** BaseRenamer class **********/
/* Constructor */
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

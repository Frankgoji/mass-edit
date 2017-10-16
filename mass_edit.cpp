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
    : files(0),
      longestName(0)
{
    listdir();
}

/* Rename files with the appropriate directory prefix */
void BaseRenamer::dir_rename(string old, string n) {
    fs::rename(fs::current_path() / old, fs::current_path() / n);
}

/* Lists the items in the directory */
vector<string> & BaseRenamer::listdir() {
    longestName = 0;
    if (files.size() != 0) {
        files = vector<string>(0);
    }
    fs::directory_iterator enditr;
    for (fs::directory_iterator diritr(fs::current_path());
            diritr != enditr;
            diritr++) {
        string s(diritr->path().filename().string());
        size_t suffixstart = s.find_last_of(".");
        if (suffixstart != string::npos && suffixstart > longestName) {
            longestName = suffixstart;
        }
        files.push_back(s);
    }
    sort(files.begin(), files.end());
    return files;
}

/* Normalize filename lengths up to numZeros */
void BaseRenamer::normalize(int numZeros) {
    for (size_t i = 0; i < files.size(); i++) {
        dir_rename(files[i], normalize(files[i], numZeros));
    }
    listdir();
}

/* Normalize this filename lengths up to numZeros */
string BaseRenamer::normalize(string filename, int numZeros) {
    stringstream name;
    size_t suffixstart = filename.find_last_of(".");
    suffixstart = (suffixstart == string::npos) ? filename.length() : suffixstart;
    name << setfill('0') << setw(numZeros) << filename.substr(0, suffixstart);
    name << filename.substr(suffixstart);
    return name.str();
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
    if ((add > 0 && fileRange.begin() < fileRange.end())
            || (add < 0 && fileRange.begin() > fileRange.end())) {
        fileRange = fileRange.reverse();
    }
    for (int i = fileRange.begin(); !fileRange.OutOfRange(i); i = fileRange.Next(i)) {
        string newFile = addAmt(files[i], add);
        if (newFile == files[i]) {
            break;
        }
        dir_rename(files[i], newFile);
    }
    listdir();
    normalize(longestName);
}

/* Checks if a file collisions will happen due to a shift. Return true if no
 * file collisions, false if there are.
 * Preconditions: fileRange is listed from lowest to largest */
bool BaseRenamer::check_shift(Range fileRange, int shift) {
    Range allFiles = Range(0, files.size());
    if (fileRange.Span() == allFiles.Span() // Same range, no file collisions
            || (shift > 0 && fileRange.end() == allFiles.end())
            || (shift < 0 && fileRange.begin() == allFiles.begin())) { // no collisions
        return true;
    }
    Range * possibleCollision;
    if (shift > 0) {
        possibleCollision = new Range(fileRange.end(), allFiles.end());
    } else {
        possibleCollision = new Range(0, fileRange.begin());
    }
    set<string> collisionNames;
    for (int i = possibleCollision->begin(); !possibleCollision->OutOfRange(i);
            i = possibleCollision->Next(i)) {
        collisionNames.insert(files[i]);
    }
    set<string>::iterator it;
    for (int i = fileRange.begin(); !fileRange.OutOfRange(i); i = fileRange.Next(i)) {
        string newFile = normalize(addAmt(files[i], shift), longestName);
        if (newFile == files[i]) {
            cerr << "File doesn't start with number." << endl;
            return false;
        }
        if (collisionNames.count(newFile) != 0) {
            return false;
        }
    }
    return true;
}

/* Add (or subtract) the given amount from the filename */
string BaseRenamer::addAmt(string filename, int amt) {
    int fileNum;
    stringstream strm(filename);
    size_t suffixstart = filename.find_last_of(".");
    if (strm >> fileNum) {
        stringstream newFile("");
        fileNum += amt;
        newFile << fileNum;
        if (newFile.str().length() > longestName) {
            longestName = newFile.str().length();
        }
        string suffix = "";
        if (suffixstart != string::npos) {
            suffix = filename.substr(suffixstart);
            newFile << suffix;
        }
        return newFile.str();
    } else {
        cerr << "File doesn't start with number." << endl;
    }
    return filename;
}

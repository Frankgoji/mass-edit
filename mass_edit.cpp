#include "mass_edit.h"

using namespace std;

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
 * The item at origpos will be moved to newpos and everything else will be
 * shifted over.
 * Precondition: files has been populated by listdir. */
void BaseRenamer::shiftnames(int origpos, int newpos) {
    // Rename origpos file to temp
    dir_rename(files[origpos], "temp");
    if (origpos < newpos) {
        for (int i = origpos; i < newpos; i++) {
            // Rename files[i+1] to files[i]
            dir_rename(files[i+1], files[i]);
        }
    } else {
        for (int i = origpos; i > newpos; i--) {
            // Rename files[i-1] to files[i]
            dir_rename(files[i-1], files[i]);
        }
    }
    // Rename origpos file to newpos
    dir_rename("temp", files[newpos]);
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
                InterpretSwitch(linestrm);
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

/* Interpret the switch command */
void CLIRenamer::InterpretSwitch(stringstream & line) {
    int old, n;
    line >> old >> n;
    shiftnames(old, n);
}

/* Quit */
void CLIRenamer::InterpretQuit() {
    exit(0);
}

/* Usage */
void CLIRenamer::InterpretHelp() {
    cout << "Usage:" << endl;
    cout << "cd <directory>\t\t\tchange directory" << endl;
    cout << "ls\t\t\t\tlist directory contents with indices" << endl;
    cout << "switch <index1> <index2>\tswitch items 1 and 2, appropriately shifting the other items" << endl;
    cout << "quit" << endl;
}

int main() {
    CLIRenamer cli;
    cli.InterpretCommands();

    return 0;
}

#include "mass_edit.h"

/********** CLIRenamer Class **********/
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
    } else if (!check_shift(*r, amt)) {  // check for a conflict
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

/* Main program; creates an instance of CLIRenamer, and starts REPL loop */
int main() {
    CLIRenamer cli;
    cli.InterpretCommands();

    return 0;
}

#include <Wt/WApplication>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WCssStyleSheet>
#include <Wt/WIntValidator>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WText>

#include <boost/algorithm/string/join.hpp>
#include <sstream>
#include <string>

#include "mass_edit.h"

#define FIRST_UNSELECTED -1
#define SELECTED -2

using namespace Wt;

/*
 * GUI interface for the rename application, allowing users to mass edit
 * numbered files with ease.
 */
class RenameApplication : public WApplication, public BaseRenamer {
    public:
        RenameApplication(const WEnvironment& env);
        WContainerWidget * response;
        WContainerWidget * tableContainer;
        WContainerWidget * controls;

    private:
        WLineEdit * directory;
        WLineEdit * shift_input;
        WLineEdit * insert_input;
        vector<WPushButton *> fileContainers;
        int first_index;
        Range range;
        bool a_pressed, ctrl_pressed;
        void retrieve_files();
        void display_files();
        void normalizeOp();
        void parse();
        void increment(int i, bool isPlus);
        void set_range(int index);
        void select_all(WKeyEvent w);
        void key_up(WKeyEvent w);
        void add_controls();
        void select_control(WContainerWidget * selected, WContainerWidget * disabled);
        void shift_gui(WLineEdit * input);
        void insert_gui(WLineEdit * input, WIntValidator * intv);
        void alert(string message);
};

/* Constructor for RenameApplication. */
RenameApplication::RenameApplication(const WEnvironment& env)
    : WApplication(env),
      BaseRenamer(),
      range(0, 0) {

    first_index = FIRST_UNSELECTED;
    a_pressed = false, ctrl_pressed = false;
    WApplication::instance()->useStyleSheet("style.css");
    WApplication::instance()->declareJavaScriptFunction("addHover", "function() { var styleSheet = document.styleSheets[1]['cssRules'][2]['styleSheet']; for (i = 0; i < styleSheet['cssRules'].length; i++) { if (styleSheet['cssRules'][i]['selectorText'] === '.files:hover') { return; } } styleSheet.insertRule('.files:hover { background-color: var(--hover-color); }', 0);}");
    WApplication::instance()->declareJavaScriptFunction("deleteHover", "function() {var styleSheet = document.styleSheets[1]['cssRules'][2]['styleSheet']; for (i = 0; i < styleSheet['cssRules'].length; i++) {if (styleSheet['cssRules'][i]['selectorText'] === '.files:hover') {styleSheet.deleteRule(i); break;}}}");
    setTitle("Mass Filename Editor");

    root()->setContentAlignment(AlignCenter);
    WText * prompt = new WText("Choose the directory with files to mass edit: ");
    root()->addWidget(prompt);
    directory = new WLineEdit(root());
    directory->setFocus();
    WPushButton * button = new WPushButton("Get files", root());

    root()->addWidget(new WBreak());
    root()->addWidget(new WBreak());

    response = new WContainerWidget(root());
    root()->addWidget(new WBreak());
    tableContainer = new WContainerWidget(root());
    root()->addWidget(new WBreak());
    controls = new WContainerWidget(root());

    button->clicked().connect(this, &RenameApplication::retrieve_files);
    directory->enterPressed().connect(this, &RenameApplication::retrieve_files);
}

/* Gets files and displays it on the page */
void RenameApplication::retrieve_files() {
    // if bad directory, produce error text and return
    // otherwise, get path, change dir, and list dirs, then create table
    response->clear();
    tableContainer->clear();
    controls->clear();
    first_index = FIRST_UNSELECTED;
    WApplication::instance()->doJavaScript(WApplication::instance()->javaScriptClass() + ".addHover()");
    WApplication::instance()->doJavaScript("document.body.style.setProperty(\"--hover-color\", \"yellow\")");
    std::string filename(directory->text().toUTF8());
    response->addWidget(new WText("Checking directory " + filename));

    try {
        fs::current_path(filename);
        listdir();
    } catch (fs::filesystem_error) {
        tableContainer->addWidget(new WText("Error: Cannot access directory " + filename));
        return;
    }

    display_files();
    tableContainer->addWidget(new WBreak());
    WPushButton * reset = new WPushButton("Reset", tableContainer);
    reset->clicked().connect(this, &RenameApplication::retrieve_files);

    WApplication::globalKeyWentDown().connect(this,
            &RenameApplication::select_all);
    WApplication::globalKeyWentUp().connect(this,
            &RenameApplication::key_up);
    tableContainer->keyWentDown().connect(this, &RenameApplication::select_all);
    tableContainer->keyWentUp().connect(this, &RenameApplication::key_up);
}

/* Displays files on the page */
void RenameApplication::display_files() {
    fileContainers = vector<WPushButton *>(0);
    bool incFound(false);
    for (size_t i = 0; i < files.size(); i++) {
        string file(files[i]);
        if (file.find("+.") != string::npos
                || file.find("-.") != string::npos) {
            incFound = true;
        }
        WPushButton * fileButton = new WPushButton(file);
        WContainerWidget * fileDiv = new WContainerWidget(tableContainer);
        fileDiv->setStyleClass("filesdiv");
        fileButton->setStyleClass("files");
        fileButton->clicked().connect(std::bind(&RenameApplication::set_range,
                    this, i));
        fileDiv->addWidget(fileButton);
        fileContainers.push_back(fileButton);
    }
    int top(0);
    if (needNormalize) {
        WContainerWidget * normContainer = new WContainerWidget();
        WPushButton * norm = new WPushButton("Normalize");
        tableContainer->insertWidget(top, normContainer);
        normContainer->addWidget(new WText("We found files that don't have the same file length. Would you like to normalize? "));
        normContainer->addWidget(norm);
        norm->clicked().connect(this, &RenameApplication::normalizeOp);
        top++;
    }
    if (incFound) {
        WContainerWidget * incContainer = new WContainerWidget();
        WPushButton * parse = new WPushButton("Parse");
        tableContainer->insertWidget(top, incContainer);
        incContainer->addWidget(new WText("We found files with + or - flags. Would you like to parse and increment/decrement? "));
        incContainer->addWidget(parse);
        parse->clicked().connect(this, &RenameApplication::parse);
    }
}

void RenameApplication::normalizeOp() {
    normalize(longestName);
    retrieve_files();
}

// Convenience utilities
enum status {PLUS, MINUS, NEITHER};
typedef struct {
    int start;
    int end;
    status flag;
    string prefix;
    int flagCount;
} groupStats;
typedef struct {
    string name;
    status flag;
    string prefix;
    int flagCount;
} fileStats;
static string getPrefix(string name) {
    stringstream res;
    string str(name);
    if (name.at(0) == '-') {
        res << "-";
        str = str.substr(1);
    }
    res << str.substr(0, str.find_first_not_of("1234567890"));
    return res.str();
}
static fileStats fileStat(string file) {
    status fileFlag(NEITHER);
    if (file.find("+.") != string::npos) {
        fileFlag = PLUS;
    } else if (file.find("-.") != string::npos) {
        fileFlag = MINUS;
    }
    string filePrefix(getPrefix(file));
    int fileFlagCount((fileFlag == NEITHER) ? 0 : count(file.begin(), file.end(), (fileFlag == PLUS) ? '+' : '-'));
    fileStats stats{file, fileFlag, filePrefix, fileFlagCount};
    return stats;
}
static void printGroupStats(groupStats g) {
    char flag((g.flag == NEITHER) ? ' ' : (g.flag == PLUS) ? '+' : '-');
    string flags(g.flagCount, flag);
}

/* Parses the files to increment or decrement them appropriately. */
void RenameApplication::parse() {
    size_t i(0);
    groupStats group{-1, -1, NEITHER, "", 0};
    printGroupStats(group);
    while (i < files.size()) {
        fileStats f(fileStat(files[i]));

        // Change state
        if (group.flag != f.flag
                || group.prefix != f.prefix
                || group.flagCount != f.flagCount) {
            group.flag = f.flag;
            group.prefix = f.prefix;
            group.flagCount = f.flagCount;
            if (f.flag == NEITHER) {
                group.end = (group.start = -1);
            } else {
                group.end = (group.start = i) + 1;
            }
        } else if (group.flag != NEITHER) {
            group.end++;
        }
        printGroupStats(group);

        // If need to increment/decrement group, do it
        if (group.flag != NEITHER) {
            // Check if next file in same group
            fileStats fNext;
            if (i < files.size() - 1) {
                fNext = fileStat(files[i+1]);
            }
            bool nextNotInGroup(fNext.flag != group.flag
                    || fNext.prefix != group.prefix
                    || fNext.flagCount != group.flagCount);
            if (i == files.size() - 1 || nextNotInGroup) {
                bool isPlus(group.flag == PLUS);
                if (isPlus) {
                    increment(group.start, isPlus);
                } else {
                    increment(group.end - 1, isPlus);
                }
                for (int i = group.start; i < group.end; i++) {
                    // remove flags before incrementing; assume that the flags
                    // are contiguous and single period in name
                    string sansFlags(files[i].substr(0, files[i].find_last_of("1234567890") + 1)
                            + files[i].substr(files[i].find_last_of(".")));
                    dir_rename(files[i], sansFlags);
                }
                listdir();
            }
        }
        i++;
    }
    retrieve_files();
}

/* Performs the appropriate increment/decrement action: if increment, shift this
 * file and all subsequent files up. If decrement, shift this file and all prior
 * files down. */
void RenameApplication::increment(int i, bool isPlus) {
    first_index = FIRST_UNSELECTED;
    int second_index(files.size() - 1);
    if (!isPlus) {
        second_index = 0;
    }
    set_range(i);
    set_range(second_index);
    WLineEdit * shift = new WLineEdit(isPlus ? "1" : "-1");
    shift_gui(shift);
}

/* Sets the range determined by the user input */
void RenameApplication::set_range(int index) {
    if (first_index == FIRST_UNSELECTED) {
        first_index = index;
        fileContainers[index]->setStyleClass("files firstinrange");
        WApplication::instance()->doJavaScript("document.body.style.setProperty(\"--hover-color\", \"red\")");
        return;
    } else if (first_index == SELECTED) {
        return;
    }
    range = Range(first_index, index+1);
    if (index < first_index) {
        range = Range(index, first_index+1);
    }
    first_index = SELECTED;
    WApplication::instance()->doJavaScript(WApplication::instance()->javaScriptClass() + ".deleteHover()");

    for (size_t i = 0; i < files.size(); i++) {
        fileContainers[i]->setStyleClass("files");
        if (!range.OutOfRange(i)) {
            fileContainers[i]->addStyleClass("selected");
        }
    }
    add_controls();
}

/* Selects all for the range */
void RenameApplication::select_all(WKeyEvent w) {
    if (w.key() == Key_A) {
        a_pressed = true;
    }
    if (w.key() == Key_Control) {
        ctrl_pressed = true;
    }
    if (a_pressed && ctrl_pressed) {
        first_index = FIRST_UNSELECTED;
        set_range(0);
        set_range(files.size() - 1);
    }
}

/* Behavior for key up */
void RenameApplication::key_up(WKeyEvent w) {
    a_pressed = false, ctrl_pressed = false;
}

/* Add controls below the selection */
void RenameApplication::add_controls() {
    controls->clear();
    controls->setStyleClass("controls");
    controls->setContentAlignment(AlignLeft);

    WContainerWidget * shiftContainer = new WContainerWidget(controls);
    WContainerWidget * insertContainer = new WContainerWidget(controls);
    shiftContainer->setStyleClass("initial");
    insertContainer->setStyleClass("initial");

    WPushButton * shift = new WPushButton("Shift");
    WPushButton * insert = new WPushButton("Insert");
    shift->setStyleClass("controlbutton");
    insert->setStyleClass("controlbutton");

    WText * shift_text = new WText("Shift selection by this amount: ");
    shift_input = new WLineEdit();
    WIntValidator * shift_int = new WIntValidator();
    shift_int->setMandatory(true);
    shift_input->setValidator(shift_int);
    shift_input->setStyleClass("rightaligned");
    WPushButton * shift_button = new WPushButton("Shift");
    shift_button->clicked().connect(std::bind(&RenameApplication::shift_gui,
                this, shift_input));
    shift_button->setStyleClass("rightaligned");

    stringstream insert_text_stream;
    insert_text_stream << "Insert selection at this index (0-" << files.size() << "): ";
    WText * insert_text = new WText(insert_text_stream.str());
    insert_input = new WLineEdit();
    WIntValidator * insert_int = new WIntValidator(0, files.size());
    insert_int->setMandatory(true);
    insert_input->setValidator(insert_int);
    insert_input->setStyleClass("rightaligned");
    WPushButton * insert_button = new WPushButton("Insert");
    insert_button->clicked().connect(std::bind(&RenameApplication::insert_gui,
                this, insert_input, insert_int));
    insert_button->setStyleClass("rightaligned");

    shift->clicked().connect(std::bind(&RenameApplication::select_control, this,
                shiftContainer, insertContainer));
    insert->clicked().connect(std::bind(&RenameApplication::select_control,
                this, insertContainer, shiftContainer));

    shiftContainer->addWidget(shift);
    shiftContainer->addWidget(shift_text);
    shiftContainer->addWidget(shift_button);
    shiftContainer->addWidget(shift_input);
    insertContainer->addWidget(insert);
    insertContainer->addWidget(insert_text);
    insertContainer->addWidget(insert_button);
    insertContainer->addWidget(insert_input);
}

/* Sets the other button to disabled mode */
void RenameApplication::select_control(WContainerWidget * selected,
        WContainerWidget * disabled) {
    selected->setStyleClass("");
    disabled->setStyleClass("disabled");
    shift_input->removeStyleClass("error");
    insert_input->removeStyleClass("error");
    shift_input->setText("");
    insert_input->setText("");
}

/* Shift range by the amount inputted */
void RenameApplication::shift_gui(WLineEdit * shift_in) {
    int shift_amount;
    stringstream text_stream;
    text_stream << shift_in->text();
    text_stream >> shift_amount;

    Range filesIndex(0, files.size());
    if (!check_shift(range, shift_amount)) {  // not shifting all, but causes a conflict
        shift_in->addStyleClass("error");
        alert("File collision illegal");
    } else {
        shiftnames(range, shift_amount);
        alert("Done!");
        retrieve_files();
    }
}

/* Insert range at the index inputted */
void RenameApplication::insert_gui(WLineEdit * insert_in,
        WIntValidator * intv) {
    WValidator::Result res = intv->validate(insert_in->text());
    if (res.message() != "") {
        insert_in->addStyleClass("error");
        alert("Insert point out of range");
        return;
    }
    int index;
    stringstream text_stream;
    text_stream << insert_in->text();
    text_stream >> index;
    Range filesIndex(0, files.size());
    if (!range.OutOfRange(index)) {
        insert_in->addStyleClass("error");
        alert("Cannot insert file into the same range");
    } else {
        insert(range, index);
        alert("Done!");
        retrieve_files();
    }
}

void RenameApplication::alert(string message) {
    stringstream func;
    func << "alert(\"" << message << "\")";
    root()->doJavaScript(func.str());
}

/*
 * You could read information from the environment to decide whether
 * the user has permission to start a new application
 */
WApplication *createApplication(const WEnvironment& env) {
    return new RenameApplication(env);
}

/* Main method for application */
int main(int argc, char **argv) {
    return WRun(argc, argv, &createApplication);
}

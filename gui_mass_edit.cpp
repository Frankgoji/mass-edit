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
        void set_range(int index);
        void select_all(WKeyEvent w);
        void key_up(WKeyEvent w);
        void add_controls();
        void select_control(WContainerWidget * selected, WContainerWidget * disabled);
        void shift_gui(WIntValidator * intv);
        void insert_gui(WIntValidator * intv);
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
    for (size_t i = 0; i < files.size(); i++) {
        string file(files[i]);
        WPushButton * fileButton = new WPushButton(file);
        fileButton->setStyleClass("files");
        fileButton->clicked().connect(std::bind(&RenameApplication::set_range,
                    this, i));
        tableContainer->addWidget(fileButton);
        fileContainers.push_back(fileButton);
    }
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
                this, shift_int));
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
                this, insert_int));
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
void RenameApplication::shift_gui(WIntValidator * intv) {
    int shift_amount;
    stringstream text_stream;
    text_stream << shift_input->text();
    text_stream >> shift_amount;

    Range filesIndex(0, files.size());
    if (!check_shift(range, shift_amount)) {  // not shifting all, but causes a conflict
        shift_input->addStyleClass("error");
        alert("File collision illegal");
    } else {
        shiftnames(range, shift_amount);
        alert("Done!");
        retrieve_files();
    }
}

/* Insert range at the index inputted */
void RenameApplication::insert_gui(WIntValidator * intv) {
    WValidator::Result res = intv->validate(insert_input->text());
    if (res.message() != "") {
        insert_input->addStyleClass("error");
        alert("Insert point out of range");
        return;
    }
    int index;
    stringstream text_stream;
    text_stream << insert_input->text();
    text_stream >> index;
    Range filesIndex(0, files.size());
    if (!range.OutOfRange(index)) {
        insert_input->addStyleClass("error");
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

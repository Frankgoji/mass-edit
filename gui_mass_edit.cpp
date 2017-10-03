#include <Wt/WApplication>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WCssStyleSheet>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WText>

#include <boost/algorithm/string/join.hpp>

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
        WLineEdit * directory;    // directory gotten by user input
        vector<WPushButton *> fileContainers;
        int first_index;
        Range range;
        bool a_pressed, ctrl_pressed;
        void retrieve_files();
        void display_files();
        void set_range(int index);
        void select_all(WKeyEvent w);
        void key_up(WKeyEvent w);
        void reset_button();
        void add_controls();
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
    reset->clicked().connect(this, &RenameApplication::reset_button);

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

/* Resets the selected files when pressed */
void RenameApplication::reset_button() {
    for (WPushButton * file : fileContainers) {
        file->setStyleClass("files");
    }
    first_index = FIRST_UNSELECTED;
    WApplication::instance()->doJavaScript(WApplication::instance()->javaScriptClass() + ".addHover()");
    WApplication::instance()->doJavaScript("document.body.style.setProperty(\"--hover-color\", \"yellow\")");
    controls->clear();
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

// TODO: add options to edit file names
void RenameApplication::add_controls() {
    // implement with button and opacity
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

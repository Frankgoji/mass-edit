#include <Wt/WApplication>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WText>
#include <Wt/WFileUpload>

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
        vector<WText *> fileContainers;
        int first_index;
        Range range;
        bool a_pressed, ctrl_pressed;
        void retrieve_files();
        void display_files();
        void set_files_style(WText * file, bool going_in, int ind);
        void set_range(int index);
        void select_all(WKeyEvent w);
        void key_up(WKeyEvent w);
        void reset_button();
};

/* Constructor for RenameApplication. */
RenameApplication::RenameApplication(const WEnvironment& env)
    : WApplication(env),
      BaseRenamer(),
      range(0, 0) {

    first_index = FIRST_UNSELECTED;
    a_pressed = false, ctrl_pressed = false;
    WApplication::instance()->useStyleSheet("style.css");
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

// TODO: ctrl-a select all
/* Gets files and displays it on the page */
void RenameApplication::retrieve_files() {
    // if bad directory, produce error text and return
    // otherwise, get path, change dir, and list dirs, then create table
    response->clear();
    tableContainer->clear();
    controls->clear();
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
    for (size_t i = 0; i < files.size(); i++) {
        string file(files[i]);
        WText * fileText = new WText(file);
        fileText->setStyleClass("files");
        fileText->mouseWentOver().connect(std::bind(
                &RenameApplication::set_files_style, this, fileText, true, i));
        fileText->mouseWentOut().connect(std::bind(
                &RenameApplication::set_files_style, this, fileText, false, i));
        fileText->clicked().connect(std::bind(&RenameApplication::set_range,
                    this, i));
        tableContainer->addWidget(fileText);
        fileContainers.push_back(fileText);
    }
}

/* Sets the style for the files buttons */
void RenameApplication::set_files_style(WText * file, bool going_in, int ind) {
    if (going_in && first_index == FIRST_UNSELECTED) {
        file->addStyleClass("firstinrange");
    } else if (ind != first_index && first_index != SELECTED) {
        if (going_in) {
            file->addStyleClass("secondinrange");
        } else {
            file->setStyleClass("files");
        }
    }
}

/* Resets the selected files when pressed */
void RenameApplication::reset_button() {
    for (WText * file : fileContainers) {
        file->setStyleClass("files");
    }
    first_index = FIRST_UNSELECTED;
}

/* Sets the range determined by the user input */
void RenameApplication::set_range(int index) {
    if (first_index == FIRST_UNSELECTED) {
        first_index = index;
        return;
    } else if (first_index == SELECTED) {
        return;
    }
    range = Range(first_index, index+1);
    if (index < first_index) {
        range = Range(index, first_index+1);
    }
    first_index = SELECTED;

    for (int i = range.begin(); !range.OutOfRange(i); i = range.Next(i)) {
        fileContainers[i]->setStyleClass("files selected");
    }
    // TODO: add options to edit file names
}

/* Selects all for the range */
void RenameApplication::select_all(WKeyEvent w) {
    if (w.key() == Key_W) {
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

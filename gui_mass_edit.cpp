#include <Wt/WApplication>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WText>
#include <Wt/WFileUpload>

#include "mass_edit.h"

using namespace Wt;

/*
 * GUI interface for the rename application, allowing users to mass edit
 * numbered files with ease.
 */
class RenameApplication : public WApplication, public BaseRenamer {
public:
    RenameApplication(const WEnvironment& env);

private:
    WLineEdit *nameEdit_;
    WText *greeting_;

    void greet();
};

// TODO: use the file upload to select all the files, and don't use the list dir
// Need to make sure files are sorted, then can use methods
/* Constructor for RenameApplication. */
RenameApplication::RenameApplication(const WEnvironment& env)
    : WApplication(env) {

    setTitle("Mass Filename Editor");                       // application title

    root()->setContentAlignment(AlignCenter);
    WText * prompt = new WText("Choose directory of files: ");
    root()->addWidget(prompt);
    WFileUpload * directory = new WFileUpload(root());
    directory->setMultiple(true);
    nameEdit_ = new WLineEdit(root());                      // allow text input
    nameEdit_->setFocus();                                  // give focus

    WPushButton *button
        = new WPushButton("Greet me.", root());             // create a button
    button->setMargin(5, Left);                             // add 5 pixels margin

    root()->addWidget(new WBreak());                        // insert a line break

    greeting_ = new WText(root());                          // empty text

    /*
     * Connect signals with slots
     *
     * - simple Wt-way
     */
    button->clicked().connect(this, &RenameApplication::greet);

    /*
     * - using an arbitrary function object (binding values with boost::bind())
     */
    nameEdit_->enterPressed().connect
      (boost::bind(&RenameApplication::greet, this));
}

/*
 * Update the text, using text input into the nameEdit_ field.
 */
void RenameApplication::greet() {
    greeting_->setText("Hello there, " + nameEdit_->text());
}

/*
 * You could read information from the environment to decide whether
 * the user has permission to start a new application
 */
WApplication *createApplication(const WEnvironment& env) {
    return new RenameApplication(env);
}

/*
 * Your main method may set up some shared resources, but should then
 * start the server application (FastCGI or httpd) that starts listening
 * for requests, and handles all of the application life cycles.
 *
 * The last argument to WRun specifies the function that will instantiate
 * new application objects. That function is executed when a new user surfs
 * to the Wt application, and after the library has negotiated browser
 * support. The function should return a newly instantiated application
 * object.
 */
int main(int argc, char **argv) {
    return WRun(argc, argv, &createApplication);
}

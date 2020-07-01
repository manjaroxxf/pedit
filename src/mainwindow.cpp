#include "mainwindow.h"

#include <KActionCollection>
#include <KMessageBox>
#include <KStandardAction>
#include <KXMLGUIFactory>

//

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSaveFile>
#include <QTextStream>

MainWindow::MainWindow(ConfigHelper *cfHelper) : configHelper(cfHelper)
{
    resize(550, 600);

    editor = KTextEditor::Editor::instance();
    doc = editor->createDocument(this);
    view = doc->createView(this);

    setCentralWidget(view);
    setupActions();

    guiFactory()->addClient(view);
    createShellGUI(true);

    show();
}

MainWindow::~MainWindow() {}

void MainWindow::setupActions()
{
    KStandardAction::open(this, SLOT(openFile()), actionCollection());
}

void MainWindow::openFile()
{
    view->document()->openUrl(QFileDialog::getOpenFileUrl());
}

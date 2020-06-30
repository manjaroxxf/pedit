#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSaveFile>
#include <QTextStream>

MainWindow::MainWindow(ConfigHelper *cfHelper)
    : configHelper(cfHelper), settingsWindow(new SettingsWindow)
{
    resize(550, 600);

    editor = KTextEditor::Editor::instance();
    doc = editor->createDocument(this);
    view = doc->createView(this);

    setCentralWidget(view);
}

MainWindow::~MainWindow() {}

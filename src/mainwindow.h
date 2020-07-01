#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/view.h>

#include <KParts/MainWindow>
#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>

#include "confighelper.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QPlainTextEdit;
QT_END_NAMESPACE

class MainWindow : public KParts::MainWindow
{
    Q_OBJECT

   public:
    MainWindow(ConfigHelper *cfHelper);
    ~MainWindow();

   private slots:
    void openFile();

   private:
    void setupActions();
    ConfigHelper *configHelper;
    KTextEditor::Editor *editor;
    KTextEditor::Document *doc;
    KTextEditor::View *view;
};
#endif  // MAINWINDOW_H

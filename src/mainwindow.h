#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/view.h>

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>

#include "confighelper.h"
#include "settingswindow.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QPlainTextEdit;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

   public:
    MainWindow(ConfigHelper *cfHelper);
    ~MainWindow();

   private:
    ConfigHelper *configHelper;
    SettingsWindow *settingsWindow;

    KTextEditor::Editor *editor;
    KTextEditor::Document *doc;
    KTextEditor::View *view;
};
#endif  // MAINWINDOW_H

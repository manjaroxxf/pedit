#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>

#include "confighelper.h"
#include "ptextedit.h"
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

   private slots:
    void newFile();
    void open();
    void save();
    void saveAs();

   private:
    ConfigHelper *configHelper;
    PTextEdit *plainTextEdit;
    SettingsWindow *settingsWindow;

    bool maybeSave();

    void setupActions();
    void setupMenus();
    QMenu *fileMenu;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *settingsAct;
    QAction *exitAct;
    QMenu *helpMenu;
    QAction *aboutQtAct;
};
#endif  // MAINWINDOW_H

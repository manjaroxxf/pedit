#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>

#include "ptextedit.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QPlainTextEdit;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

   public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

   private slots:
    void newFile();
    void open();
    void save();
    void saveAs();

   private:
    PTextEdit *plainTextEdit;

    bool maybeSave();

    void setupActions();
    void setupMenus();
    QMenu *fileMenu;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QMenu *helpMenu;
    QAction *aboutQtAct;
};
#endif  // MAINWINDOW_H

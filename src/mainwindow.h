#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>

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

 private:
  QPlainTextEdit *plainTextEdit;

  void setupActions();
  void setupMenus();
  QMenu *fileMenu;
  QAction *newAct;
  QAction *openAct;
};
#endif  // MAINWINDOW_H

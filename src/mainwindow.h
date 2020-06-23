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

 private slots:
  void newFile();
  void open();
  void save();
  void saveAs();

 private:
  QPlainTextEdit *plainTextEdit;
  QString currentFile;

  bool maybeSave();
  void setCurrentFile(const QString &fileName);
  void saveFile(const QString &fileName);

  void setupActions();
  void setupMenus();
  QMenu *fileMenu;
  QAction *newAct;
  QAction *openAct;
  QAction *saveAct;
  QAction *saveAsAct;
  QAction *exitAct;
};
#endif  // MAINWINDOW_H

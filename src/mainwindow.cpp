#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSaveFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), plainTextEdit(new PTextEdit)
{
  resize(550, 600);
  setCentralWidget(plainTextEdit);

  setupActions();
  setupMenus();
}

MainWindow::~MainWindow() { delete plainTextEdit; }

void MainWindow::newFile()
{
  if (maybeSave()) {
    plainTextEdit->clear();
    plainTextEdit->setCurrentFile(QString());
    setWindowFilePath("Untitled.txt");
  }
}

void MainWindow::open()
{
  if (maybeSave()) {
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) {
      plainTextEdit->openFile(fileName);
      setWindowFilePath(fileName);
    }
  }
}

void MainWindow::save()
{
  if (plainTextEdit->currentFile.isEmpty())
    saveAs();
  else
    plainTextEdit->saveFile(plainTextEdit->currentFile);
}

void MainWindow::saveAs()
{
  QFileDialog dialog(this);
  dialog.setWindowModality(Qt::WindowModal);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  if (dialog.exec() == QDialog::Accepted) {
    const QString fileName = dialog.selectedFiles().first();
    plainTextEdit->saveFile(fileName);
    setWindowFilePath(fileName);
  }
}

bool MainWindow::maybeSave()
{
  if (!plainTextEdit->document()->isModified()) return true;
  const QMessageBox::StandardButton ret = QMessageBox::warning(
      this, tr("Application"),
      tr("The document has been modified.\n"
         "Do you want to save your changes?"),
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
  switch (ret) {
    case QMessageBox::Save:
      save();
      return true;
    case QMessageBox::Cancel:
      return false;
    default:
      break;
  }
  return true;
}

void MainWindow::setupActions()
{
  newAct = new QAction(QIcon(":/icons/assets/icons/add.svg"), tr("&New"), this);
  newAct->setShortcuts(QKeySequence::New);
  newAct->setStatusTip(tr("Create a new file"));
  connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

  openAct = new QAction(QIcon(":/icons/assets/icons/open_in_full.svg"),
                        tr("&Open"), this);
  openAct->setShortcut(QKeySequence::Open);
  openAct->setStatusTip(tr("Open a file"));
  connect(openAct, &QAction::triggered, this, &MainWindow::open);

  saveAct =
      new QAction(QIcon(":/icons/assets/icons/save.svg"), tr("&Save"), this);
  saveAct->setShortcut(QKeySequence::Save);
  saveAct->setStatusTip(tr("Save a file"));
  connect(saveAct, &QAction::triggered, this, &MainWindow::save);

  saveAsAct =
      new QAction(QIcon(":/icons/assets/icons/save.svg"), tr("&Save as"), this);
  saveAsAct->setShortcut(QKeySequence::SaveAs);
  connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);

  exitAct = new QAction(QIcon(":/icons/assets/icons/exit_to_app.svg"),
                        tr("&Exit"), this);
  exitAct->setShortcut(QKeySequence::Quit);
  exitAct->setStatusTip(tr("Exit panda-note"));
  connect(exitAct, &QAction::triggered, this, &QApplication::closeAllWindows);

  aboutQtAct = new QAction(tr("About Qt"));
  aboutQtAct->setStatusTip(tr("About Qt"));
  connect(aboutQtAct, &QAction::triggered, this, &QApplication::aboutQt);
}

void MainWindow::setupMenus()
{
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAct);
  fileMenu->addAction(openAct);
  fileMenu->addAction(saveAct);
  fileMenu->addAction(saveAsAct);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAct);

  helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(aboutQtAct);
}

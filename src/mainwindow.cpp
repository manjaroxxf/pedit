#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), plainTextEdit(new QPlainTextEdit)
{
  resize(550, 600);
  setCentralWidget(plainTextEdit);

  setupActions();
  setupMenus();
}

MainWindow::~MainWindow() { delete plainTextEdit; }

void MainWindow::setupActions()
{
  newAct = new QAction(QIcon(":/icons/assets/icons/add.svg"), tr("&New"), this);
  newAct->setShortcuts(QKeySequence::New);
  newAct->setStatusTip(tr("Create a new file"));

  openAct = new QAction(QIcon(":/icons/assets/icons/open_in_full.svg"),
                        tr("&Open"), this);
  openAct->setShortcut(QKeySequence::Open);
  newAct->setStatusTip(tr("Open a file"));
}

void MainWindow::setupMenus()
{
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAct);
  fileMenu->addAction(openAct);
}

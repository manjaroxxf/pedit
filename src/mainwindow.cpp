#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), plainTextEdit(new QPlainTextEdit)
{
  this->setCentralWidget(plainTextEdit);
}

MainWindow::~MainWindow() { delete plainTextEdit; }

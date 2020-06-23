#include "mainwindow.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSaveFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), plainTextEdit(new QPlainTextEdit)
{
  resize(550, 600);
  setCentralWidget(plainTextEdit);

  plainTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

  setupActions();
  setupMenus();
}

MainWindow::~MainWindow() { delete plainTextEdit; }

void MainWindow::newFile()
{
  if (maybeSave()) {
    plainTextEdit->clear();
    setCurrentFile(QString());
  }
}

void MainWindow::open()
{
  if (maybeSave()) {
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) {
      QFile file(fileName);
      if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(
            this, tr("Application"),
            tr("Cannot read file %1:\n%2.")
                .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
      }

      QTextStream in(&file);

      QGuiApplication::setOverrideCursor(Qt::WaitCursor);

      plainTextEdit->setPlainText(in.readAll());

      QGuiApplication::restoreOverrideCursor();

      setCurrentFile(fileName);
    }
  }
}

void MainWindow::save()
{
  if (currentFile.isEmpty())
    saveAs();
  else
    saveFile(currentFile);
}

void MainWindow::saveAs()
{
  QFileDialog dialog(this);
  dialog.setWindowModality(Qt::WindowModal);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  if (dialog.exec() == QDialog::Accepted)
    saveFile(dialog.selectedFiles().first());
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

void MainWindow::setCurrentFile(const QString &fileName)
//! [46] //! [47]
{
  currentFile = fileName;
  plainTextEdit->document()->setModified(false);
  setWindowModified(false);

  if (currentFile.isEmpty())
    setWindowFilePath("Untitled.txt");
  else
    setWindowFilePath(currentFile);
}

void MainWindow::saveFile(const QString &fileName)
{
  QString errorMessage;

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
  QSaveFile file(fileName);
  if (file.open(QFile::WriteOnly | QFile::Text)) {
    QTextStream out(&file);
    out << plainTextEdit->toPlainText();
    if (!file.commit()) {
      errorMessage =
          tr("Cannot write file %1:\n%2.")
              .arg(QDir::toNativeSeparators(fileName), file.errorString());
    }
  } else {
    errorMessage =
        tr("Cannot open file %1 for writing:\n%2.")
            .arg(QDir::toNativeSeparators(fileName), file.errorString());
  }
  QGuiApplication::restoreOverrideCursor();

  if (!errorMessage.isEmpty()) {
    QMessageBox::warning(this, tr("Application"), errorMessage);
    qDebug() << "WARN: " << errorMessage;
    return;
  } else {
    setCurrentFile(fileName);
  }
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
  connect(exitAct, &QAction::triggered, this, &QWidget::close);
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
}

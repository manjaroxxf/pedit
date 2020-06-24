#include "ptextedit.h"

#include <definition.h>
#include <foldingregion.h>
#include <syntaxhighlighter.h>
#include <theme.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSaveFile>

PTextEdit::PTextEdit()
    : m_highlighter(new KSyntaxHighlighting::SyntaxHighlighter(document()))
{
  setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  setTheme((palette().color(QPalette::Base).lightness() < 128)
               ? m_repository.defaultTheme(
                     KSyntaxHighlighting::Repository::DarkTheme)
               : m_repository.defaultTheme(
                     KSyntaxHighlighting::Repository::LightTheme));
}

void PTextEdit::openFile(const QString &fileName)
{
  QGuiApplication::setOverrideCursor(Qt::WaitCursor);

  QFile f(fileName);
  if (!f.open(QFile::ReadOnly)) {
    qWarning() << "[WARN] Failed to open" << fileName << ":" << f.errorString();
    return;
  }

  clear();

  setPlainText(QString::fromUtf8(f.readAll()));
  setCurrentFile(fileName);

  QGuiApplication::restoreOverrideCursor();
}

void PTextEdit::saveFile(const QString &fileName)
{
  QString errorMessage;

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
  QSaveFile file(fileName);
  if (file.open(QFile::WriteOnly | QFile::Text)) {
    QTextStream out(&file);
    out << toPlainText();
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

void PTextEdit::setCurrentFile(const QString &fileName)
{
  currentFile = fileName;
  document()->setModified(false);
  setWindowModified(false);

  const auto def = m_repository.definitionForFileName(fileName);
  m_highlighter->setDefinition(def);

  if (currentFile.isEmpty())
    setWindowFilePath("Untitled.txt");
  else
    setWindowFilePath(currentFile);
}

void PTextEdit::highlightCurrentLine()
{
  QTextEdit::ExtraSelection selection;
  selection.format.setBackground(QColor(m_highlighter->theme().editorColor(
      KSyntaxHighlighting::Theme::CurrentLine)));
  selection.format.setProperty(QTextFormat::FullWidthSelection, true);
  selection.cursor = textCursor();
  selection.cursor.clearSelection();

  QList<QTextEdit::ExtraSelection> extraSelections;
  extraSelections.append(selection);
  setExtraSelections(extraSelections);
}

void PTextEdit::setTheme(const KSyntaxHighlighting::Theme &theme)
{
  auto pal = qApp->palette();
  if (theme.isValid()) {
    pal.setColor(
        QPalette::Base,
        theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
    pal.setColor(QPalette::Highlight,
                 theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
  }
  setPalette(pal);

  m_highlighter->setTheme(theme);
  m_highlighter->rehighlight();
  // TODO: highlightCurrentLine();
}

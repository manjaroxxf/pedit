#ifndef PTEXTEDIT_H
#define PTEXTEDIT_H

#include <repository.h>

#include <QPlainTextEdit>

namespace KSyntaxHighlighting
{
class SyntaxHighlighter;
}

class PTextEdit : public QPlainTextEdit
{
  Q_OBJECT
 public:
  PTextEdit();

  QString currentFile;

  void openFile(const QString &fileName);
  void saveFile(const QString &fileName);
  void setCurrentFile(const QString &fileName);

 private:
  void highlightCurrentLine();
  void setTheme(const KSyntaxHighlighting::Theme &theme);
  KSyntaxHighlighting::Repository m_repository;
  KSyntaxHighlighting::SyntaxHighlighter *m_highlighter;
};

#endif  // PTEXTEDIT_H

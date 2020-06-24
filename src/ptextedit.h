#ifndef PTEXTEDIT_H
#define PTEXTEDIT_H

#include <repository.h>

#include <QMenu>
#include <QPlainTextEdit>

namespace KSyntaxHighlighting
{
class SyntaxHighlighter;
}

class PTextEditSidebar;

class PTextEdit : public QPlainTextEdit
{
  Q_OBJECT
 public:
  PTextEdit();

  QString currentFile;

  void openFile(const QString &fileName);
  void saveFile(const QString &fileName);
  void setCurrentFile(const QString &fileName);

 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

 private:
  friend class PTextEditSidebar;

  int sidebarWidth() const;
  void sidebarPaintEvent(QPaintEvent *event);
  void updateSidebarGeometry();
  void updateSidebarArea(const QRect &rect, int dy);

  QTextBlock blockAtPosition(int y) const;
  bool isFoldable(const QTextBlock &block) const;
  bool isFolded(const QTextBlock &block) const;
  void toggleFold(const QTextBlock &block);

  void highlightCurrentLine();
  void setTheme(const KSyntaxHighlighting::Theme &theme);
  KSyntaxHighlighting::Repository m_repository;
  KSyntaxHighlighting::SyntaxHighlighter *m_highlighter;
  PTextEditSidebar *sidebar;
};

class PTextEditSidebar : public QWidget
{
  Q_OBJECT
 public:
  explicit PTextEditSidebar(PTextEdit *pTextEdit);
  QSize sizeHint() const override;

 protected:
  void paintEvent(QPaintEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

 private:
  PTextEdit *m_pTextEdit;
};

#endif  // PTEXTEDIT_H

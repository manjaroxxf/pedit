#include "ptextedit.h"

#include <definition.h>
#include <foldingregion.h>
#include <syntaxhighlighter.h>
#include <theme.h>

#include <QActionGroup>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPainter>
#include <QSaveFile>
#include <QSize>

PTextEditSidebar::PTextEditSidebar(PTextEdit *pTextEdit)
    : QWidget(pTextEdit), m_pTextEdit(pTextEdit)
{
}

QSize PTextEditSidebar::sizeHint() const
{
    return QSize(m_pTextEdit->sidebarWidth(), 0);
}

void PTextEditSidebar::paintEvent(QPaintEvent *event)
{
    m_pTextEdit->sidebarPaintEvent(event);
}

void PTextEditSidebar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->x() >= width() - m_pTextEdit->fontMetrics().lineSpacing()) {
        auto block = m_pTextEdit->blockAtPosition(event->y());
        if (!block.isValid() || !m_pTextEdit->isFoldable(block)) return;
        m_pTextEdit->toggleFold(block);
    }
    QWidget::mouseReleaseEvent(event);
}

PTextEdit::PTextEdit(ConfigHelper *cfHelper)
    : configHelper(cfHelper),
      m_highlighter(new KSyntaxHighlighting::SyntaxHighlighter(document())),
      sidebar(new PTextEditSidebar(this))
{
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setTheme((palette().color(QPalette::Base).lightness() < 128)
                 ? m_repository.defaultTheme(
                       KSyntaxHighlighting::Repository::DarkTheme)
                 : m_repository.defaultTheme(
                       KSyntaxHighlighting::Repository::LightTheme));

    connect(this, &QPlainTextEdit::blockCountChanged, this,
            &PTextEdit::updateSidebarGeometry);
    connect(this, &QPlainTextEdit::updateRequest, this,
            &PTextEdit::updateSidebarArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this,
            &PTextEdit::highlightCurrentLine);

    updateSidebarGeometry();
    highlightCurrentLine();
}

void PTextEdit::openFile(const QString &fileName)
{
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    QFile f(fileName);
    if (!f.open(QFile::ReadOnly)) {
        qWarning() << "[WARN] Failed to open" << fileName << ":"
                   << f.errorString();
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
            errorMessage = tr("Cannot write file %1:\n%2.")
                               .arg(QDir::toNativeSeparators(fileName),
                                    file.errorString());
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

void PTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    auto menu = createStandardContextMenu(event->pos());
    menu->addSeparator();
    auto openAction = menu->addAction(QStringLiteral("Open File..."));
    connect(openAction, &QAction::triggered, this, [this]() {
        const auto fileName =
            QFileDialog::getOpenFileName(this, QStringLiteral("Open File"));
        if (!fileName.isEmpty()) openFile(fileName);
    });

    // syntax selection
    auto hlActionGroup = new QActionGroup(menu);
    hlActionGroup->setExclusive(true);
    auto hlGroupMenu = menu->addMenu(QStringLiteral("Syntax"));
    QMenu *hlSubMenu = hlGroupMenu;
    QString currentGroup;
    for (const auto &def : m_repository.definitions()) {
        if (def.isHidden()) continue;
        if (currentGroup != def.section()) {
            currentGroup = def.section();
            hlSubMenu = hlGroupMenu->addMenu(def.translatedSection());
        }

        Q_ASSERT(hlSubMenu);
        auto action = hlSubMenu->addAction(def.translatedName());
        action->setCheckable(true);
        action->setData(def.name());
        hlActionGroup->addAction(action);
        if (def.name() == m_highlighter->definition().name())
            action->setChecked(true);
    }
    connect(hlActionGroup, &QActionGroup::triggered, this,
            [this](QAction *action) {
                const auto defName = action->data().toString();
                const auto def = m_repository.definitionForName(defName);
                m_highlighter->setDefinition(def);
            });

    // theme selection
    auto themeGroup = new QActionGroup(menu);
    themeGroup->setExclusive(true);
    auto themeMenu = menu->addMenu(QStringLiteral("Theme"));
    for (const auto &theme : m_repository.themes()) {
        auto action = themeMenu->addAction(theme.translatedName());
        action->setCheckable(true);
        action->setData(theme.name());
        themeGroup->addAction(action);
        if (theme.name() == m_highlighter->theme().name())
            action->setChecked(true);
    }
    connect(themeGroup, &QActionGroup::triggered, this,
            [this](QAction *action) {
                const auto themeName = action->data().toString();
                const auto theme = m_repository.theme(themeName);
                setTheme(theme);
            });

    menu->exec(event->globalPos());
    delete menu;
}

void PTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateSidebarGeometry();
}

int PTextEdit::sidebarWidth() const
{
    int digits = 1;
    auto count = blockCount();
    while (count >= 10) {
        ++digits;
        count /= 10;
    }
    return 15 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits +
         fontMetrics().lineSpacing();
}

void PTextEdit::sidebarPaintEvent(QPaintEvent *event)
{
    QPainter painter(sidebar);
    painter.fillRect(event->rect(),
                     m_highlighter->theme().editorColor(
                         KSyntaxHighlighting::Theme::IconBorder));

    auto block = firstVisibleBlock();
    auto blockNumber = block.blockNumber();
    int top = blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + blockBoundingRect(block).height();
    const int currentBlockNumber = textCursor().blockNumber();

    const auto foldingMarkerSize = fontMetrics().lineSpacing();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const auto number = QString::number(blockNumber + 1);
            painter.setPen(m_highlighter->theme().editorColor(
                (blockNumber == currentBlockNumber)
                    ? KSyntaxHighlighting::Theme::CurrentLineNumber
                    : KSyntaxHighlighting::Theme::LineNumbers));
            painter.drawText(0, top, sidebar->width() - 2 - foldingMarkerSize,
                             fontMetrics().height(), Qt::AlignRight, number);
        }

        // folding marker
        if (block.isVisible() && isFoldable(block)) {
            QPolygonF polygon;
            if (isFolded(block)) {
                polygon << QPointF(foldingMarkerSize * 0.4,
                                   foldingMarkerSize * 0.25);
                polygon << QPointF(foldingMarkerSize * 0.4,
                                   foldingMarkerSize * 0.75);
                polygon << QPointF(foldingMarkerSize * 0.8,
                                   foldingMarkerSize * 0.5);
            } else {
                polygon << QPointF(foldingMarkerSize * 0.25,
                                   foldingMarkerSize * 0.4);
                polygon << QPointF(foldingMarkerSize * 0.75,
                                   foldingMarkerSize * 0.4);
                polygon << QPointF(foldingMarkerSize * 0.5,
                                   foldingMarkerSize * 0.8);
            }
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(m_highlighter->theme().editorColor(
                KSyntaxHighlighting::Theme::CodeFolding)));
            painter.translate(sidebar->width() - foldingMarkerSize, top);
            painter.drawPolygon(polygon);
            painter.restore();
        }

        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void PTextEdit::updateSidebarGeometry()
{
    setViewportMargins(sidebarWidth(), 0, 0, 0);
    const auto r = contentsRect();
    sidebar->setGeometry(QRect(r.left(), r.top(), sidebarWidth(), r.height()));
}

void PTextEdit::updateSidebarArea(const QRect &rect, int dy)
{
    if (dy)
        sidebar->scroll(0, dy);
    else
        sidebar->update(0, rect.y(), sidebar->width(), rect.height());
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
        pal.setColor(
            QPalette::Highlight,
            theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
    }
    setPalette(pal);

    m_highlighter->setTheme(theme);
    m_highlighter->rehighlight();
    highlightCurrentLine();
}

QTextBlock PTextEdit::blockAtPosition(int y) const
{
    auto block = firstVisibleBlock();
    if (!block.isValid()) return QTextBlock();

    int top = blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + blockBoundingRect(block).height();
    do {
        if (top <= y && y <= bottom) return block;
        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
    } while (block.isValid());
    return QTextBlock();
}

bool PTextEdit::isFoldable(const QTextBlock &block) const
{
    return m_highlighter->startsFoldingRegion(block);
}

bool PTextEdit::isFolded(const QTextBlock &block) const
{
    if (!block.isValid()) return false;
    const auto nextBlock = block.next();
    if (!nextBlock.isValid()) return false;
    return !nextBlock.isVisible();
}

void PTextEdit::toggleFold(const QTextBlock &startBlock)
{
    // we also want to fold the last line of the region, therefore the ".next()"
    const auto endBlock =
        m_highlighter->findFoldingRegionEnd(startBlock).next();

    if (isFolded(startBlock)) {
        // unfold
        auto block = startBlock.next();
        while (block.isValid() && !block.isVisible()) {
            block.setVisible(true);
            block.setLineCount(block.layout()->lineCount());
            block = block.next();
        }

    } else {
        // fold
        auto block = startBlock.next();
        while (block.isValid() && block != endBlock) {
            block.setVisible(false);
            block.setLineCount(0);
            block = block.next();
        }
    }

    // redraw document
    document()->markContentsDirty(
        startBlock.position(), endBlock.position() - startBlock.position() + 1);

    // update scrollbars
    emit document()->documentLayout()->documentSizeChanged(
        document()->documentLayout()->documentSize());
}

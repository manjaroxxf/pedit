/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2002, 2003, 2004 Anders Lund <anders.lund@lund.tdcadsl.dk>
    Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "katebookmarks.h"

#include "kateabstractinputmode.h"
#include "katedocument.h"
#include "kateview.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KGuiItem>
#include <KLocalizedString>
#include <KStringHandler>
#include <KToggleAction>
#include <KXMLGUIClient>
#include <KXMLGUIFactory>

#include <QEvent>
#include <QRegularExpression>
#include <QVector>

namespace KTextEditor
{
class Document;
}

KateBookmarks::KateBookmarks(KTextEditor::ViewPrivate *view, Sorting sort)
    : QObject(view)
    , m_view(view)
    , m_bookmarkClear(nullptr)
    , m_sorting(sort)
{
    setObjectName(QStringLiteral("kate bookmarks"));
    connect(view->doc(), SIGNAL(marksChanged(KTextEditor::Document *)), this, SLOT(marksChanged()));
    _tries = 0;
    m_bookmarksMenu = nullptr;
}

KateBookmarks::~KateBookmarks()
{
}

void KateBookmarks::createActions(KActionCollection *ac)
{
    m_bookmarkToggle = new KToggleAction(i18n("Set &Bookmark"), this);
    ac->addAction(QStringLiteral("bookmarks_toggle"), m_bookmarkToggle);
    m_bookmarkToggle->setIcon(QIcon::fromTheme(QStringLiteral("bookmark-new")));
    ac->setDefaultShortcut(m_bookmarkToggle, Qt::CTRL + Qt::Key_B);
    m_bookmarkToggle->setWhatsThis(i18n("If a line has no bookmark then add one, otherwise remove it."));
    connect(m_bookmarkToggle, SIGNAL(triggered()), this, SLOT(toggleBookmark()));

    m_bookmarkClear = new QAction(i18n("Clear &All Bookmarks"), this);
    ac->addAction(QStringLiteral("bookmarks_clear"), m_bookmarkClear);
    m_bookmarkClear->setIcon(QIcon::fromTheme(QStringLiteral("bookmark-remove")));
    m_bookmarkClear->setWhatsThis(i18n("Remove all bookmarks of the current document."));
    connect(m_bookmarkClear, SIGNAL(triggered()), this, SLOT(clearBookmarks()));

    m_goNext = new QAction(i18n("Next Bookmark"), this);
    ac->addAction(QStringLiteral("bookmarks_next"), m_goNext);
    m_goNext->setIcon(QIcon::fromTheme(QStringLiteral("go-down-search")));
    ac->setDefaultShortcut(m_goNext, Qt::ALT + Qt::Key_PageDown);
    m_goNext->setWhatsThis(i18n("Go to the next bookmark."));
    connect(m_goNext, SIGNAL(triggered()), this, SLOT(goNext()));

    m_goPrevious = new QAction(i18n("Previous Bookmark"), this);
    ac->addAction(QStringLiteral("bookmarks_previous"), m_goPrevious);
    m_goPrevious->setIcon(QIcon::fromTheme(QStringLiteral("go-up-search")));
    ac->setDefaultShortcut(m_goPrevious, Qt::ALT + Qt::Key_PageUp);
    m_goPrevious->setWhatsThis(i18n("Go to the previous bookmark."));
    connect(m_goPrevious, SIGNAL(triggered()), this, SLOT(goPrevious()));

    KActionMenu *actionMenu = new KActionMenu(i18n("&Bookmarks"), this);
    ac->addAction(QStringLiteral("bookmarks"), actionMenu);
    actionMenu->setDelayed(false);
    m_bookmarksMenu = actionMenu->menu();

    connect(m_bookmarksMenu, SIGNAL(aboutToShow()), this, SLOT(bookmarkMenuAboutToShow()));

    marksChanged();

    // Always want the actions with shortcuts plugged into something so their shortcuts can work
    m_view->addAction(m_bookmarkToggle);
    m_view->addAction(m_bookmarkClear);
    m_view->addAction(m_goNext);
    m_view->addAction(m_goPrevious);
}

void KateBookmarks::toggleBookmark()
{
    uint mark = m_view->doc()->mark(m_view->cursorPosition().line());
    if (mark & KTextEditor::MarkInterface::markType01)
        m_view->doc()->removeMark(m_view->cursorPosition().line(), KTextEditor::MarkInterface::markType01);
    else
        m_view->doc()->addMark(m_view->cursorPosition().line(), KTextEditor::MarkInterface::markType01);
}

void KateBookmarks::clearBookmarks()
{
    QHash<int, KTextEditor::Mark *> m = m_view->doc()->marks();
    for (QHash<int, KTextEditor::Mark *>::const_iterator i = m.constBegin(); i != m.constEnd(); ++i) {
        m_view->doc()->removeMark(i.value()->line, KTextEditor::MarkInterface::markType01);
    }

    // just to be sure ;)
    // dominik: the following line can be deleted afaics, as Document::removeMark emits this signal.
    marksChanged();
}

void KateBookmarks::insertBookmarks(QMenu &menu)
{
    const int line = m_view->cursorPosition().line();
    const QRegularExpression re(QStringLiteral("&(?!&)"));
    int next = -1; // -1 means next bookmark doesn't exist
    int prev = -1; // -1 means previous bookmark doesn't exist

    const QHash<int, KTextEditor::Mark *> &m = m_view->doc()->marks();
    QVector<int> bookmarkLineArray; // Array of line numbers which have bookmarks

    if (m.isEmpty()) {
        return;
    }

    // Find line numbers where bookmarks are set & store those line numbers in bookmarkLineArray
    for (QHash<int, KTextEditor::Mark *>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it) {
        if (it.value()->type & KTextEditor::MarkInterface::markType01) {
            bookmarkLineArray.append(it.value()->line);
        }
    }

    if (m_sorting == Position) {
        std::sort(bookmarkLineArray.begin(), bookmarkLineArray.end());
    }

    QAction *firstNewAction = menu.addSeparator();
    // Consider each line with a bookmark one at a time
    for (int i = 0; i < bookmarkLineArray.size(); ++i) {
        // Get text in this particular line in a QString
        QString bText = menu.fontMetrics().elidedText(m_view->doc()->line(bookmarkLineArray.at(i)), Qt::ElideRight, menu.fontMetrics().maxWidth() * 32);
        bText.replace(re, QStringLiteral("&&"));            // kill undesired accellerators!
        bText.replace(QLatin1Char('\t'), QLatin1Char(' ')); // kill tabs, as they are interpreted as shortcuts

        QAction *before = nullptr;
        if (m_sorting == Position) {
            // 3 actions already present
            if (menu.actions().size() <= i + 3) {
                before = nullptr;
            } else {
                before = menu.actions().at(i + 3);
            }
        }

        // Adding action for this bookmark in menu
        if (before) {
            QAction *a = new QAction(QStringLiteral("%1  %3  - \"%2\"").arg(bookmarkLineArray.at(i) + 1).arg(bText).arg(m_view->currentInputMode()->bookmarkLabel(bookmarkLineArray.at(i))), &menu);
            menu.insertAction(before, a);
            connect(a, SIGNAL(activated()), this, SLOT(gotoLine()));
            a->setData(bookmarkLineArray.at(i));
            if (!firstNewAction) {
                firstNewAction = a;
            }

        } else {
            QAction *a = menu.addAction(QStringLiteral("%1  %3  - \"%2\"").arg(bookmarkLineArray.at(i) + 1).arg(bText).arg(m_view->currentInputMode()->bookmarkLabel(bookmarkLineArray.at(i))), this, SLOT(gotoLine()));
            a->setData(bookmarkLineArray.at(i));
        }

        // Find the line number of previous & next bookmark (if present) in relation to the cursor
        if (bookmarkLineArray.at(i) < line) {
            if ((prev == -1) || prev < (bookmarkLineArray.at(i))) {
                prev = bookmarkLineArray.at(i);
            }
        } else if (bookmarkLineArray.at(i) > line) {
            if ((next == -1) || next > (bookmarkLineArray.at(i))) {
                next = bookmarkLineArray.at(i);
            }
        }
    }

    if (next != -1) {
        // Insert action for next bookmark
        m_goNext->setText(i18n("&Next: %1 - \"%2\"", next + 1, KStringHandler::rsqueeze(m_view->doc()->line(next), 24)));
        menu.insertAction(firstNewAction, m_goNext);
        firstNewAction = m_goNext;
    }
    if (prev != -1) {
        // Insert action for previous bookmark
        m_goPrevious->setText(i18n("&Previous: %1 - \"%2\"", prev + 1, KStringHandler::rsqueeze(m_view->doc()->line(prev), 24)));
        menu.insertAction(firstNewAction, m_goPrevious);
        firstNewAction = m_goPrevious;
    }

    if (next != -1 || prev != -1) {
        menu.insertSeparator(firstNewAction);
    }
}

void KateBookmarks::gotoLine()
{
    if (!sender()) {
        return;
    }
    gotoLine(((QAction *)(sender()))->data().toInt());
}

void KateBookmarks::gotoLine(int line)
{
    m_view->setCursorPosition(KTextEditor::Cursor(line, 0));
}

void KateBookmarks::bookmarkMenuAboutToShow()
{
    m_bookmarksMenu->clear();
    m_bookmarkToggle->setChecked(m_view->doc()->mark(m_view->cursorPosition().line()) & KTextEditor::MarkInterface::markType01);
    m_bookmarksMenu->addAction(m_bookmarkToggle);
    m_bookmarksMenu->addAction(m_bookmarkClear);

    m_goNext->setText(i18n("Next Bookmark"));
    m_goPrevious->setText(i18n("Previous Bookmark"));

    insertBookmarks(*m_bookmarksMenu);
}

void KateBookmarks::goNext()
{
    const QHash<int, KTextEditor::Mark *> &m = m_view->doc()->marks();
    if (m.isEmpty()) {
        return;
    }

    int line = m_view->cursorPosition().line();
    int found = -1;

    for (QHash<int, KTextEditor::Mark *>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it) {
        if ((it.value()->line > line) && ((found == -1) || (found > it.value()->line))) {
            found = it.value()->line;
        }
    }

    if (found != -1) {
        gotoLine(found);
    }
}

void KateBookmarks::goPrevious()
{
    const QHash<int, KTextEditor::Mark *> &m = m_view->doc()->marks();
    if (m.isEmpty()) {
        return;
    }

    int line = m_view->cursorPosition().line();
    int found = -1;

    for (QHash<int, KTextEditor::Mark *>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it) {
        if ((it.value()->line < line) && ((found == -1) || (found < it.value()->line))) {
            found = it.value()->line;
        }
    }

    if (found != -1) {
        gotoLine(found);
    }
}

void KateBookmarks::marksChanged()
{
    if (m_bookmarkClear) {
        m_bookmarkClear->setEnabled(!m_view->doc()->marks().isEmpty());
    }
}

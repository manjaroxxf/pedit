/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) KDE Developers

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

#include "katenormalinputmode.h"
#include "katecompletionwidget.h"
#include "kateconfig.h"
#include "katesearchbar.h"
#include "kateviewinternal.h"

#include <KLocalizedString>

KateNormalInputMode::KateNormalInputMode(KateViewInternal *viewInternal)
    : KateAbstractInputMode(viewInternal)
    , m_searchBar(nullptr)
    , m_cmdLine(nullptr)
{
}

KateNormalInputMode::~KateNormalInputMode()
{
    delete m_cmdLine;
    delete m_searchBar;
}

void KateNormalInputMode::activate()
{
    view()->activateEditActions();
}

void KateNormalInputMode::deactivate()
{
    view()->deactivateEditActions();
}

void KateNormalInputMode::reset()
{
    // nothing todo
}

bool KateNormalInputMode::overwrite() const
{
    return view()->doc()->config()->ovr();
}

void KateNormalInputMode::overwrittenChar(const QChar &)
{
    // nothing todo
}

void KateNormalInputMode::clearSelection()
{
    view()->clearSelection();
}

bool KateNormalInputMode::stealKey(QKeyEvent *)
{
    return false;
}

KTextEditor::View::InputMode KateNormalInputMode::viewInputMode() const
{
    return KTextEditor::View::NormalInputMode;
}

QString KateNormalInputMode::viewInputModeHuman() const
{
    return i18n("Normal");
}

KTextEditor::View::ViewMode KateNormalInputMode::viewMode() const
{
    return view()->isOverwriteMode() ? KTextEditor::View::NormalModeOverwrite : KTextEditor::View::NormalModeInsert;
}

QString KateNormalInputMode::viewModeHuman() const
{
    return view()->isOverwriteMode() ? i18n("OVERWRITE") : i18n("INSERT");
}

void KateNormalInputMode::gotFocus()
{
    view()->activateEditActions();
}

void KateNormalInputMode::lostFocus()
{
    view()->deactivateEditActions();
}

void KateNormalInputMode::readSessionConfig(const KConfigGroup &)
{
    // do nothing
}

void KateNormalInputMode::writeSessionConfig(KConfigGroup &)
{
    // do nothing
}

void KateNormalInputMode::updateConfig()
{
    // do nothing
}

void KateNormalInputMode::readWriteChanged(bool)
{
    // inform search bar
    if (m_searchBar) {
        m_searchBar->slotReadWriteChanged();
    }
}

void KateNormalInputMode::find()
{
    KateSearchBar *const bar = searchBar(IncrementalSearchBar);
    view()->bottomViewBar()->addBarWidget(bar);
    view()->bottomViewBar()->showBarWidget(bar);
    bar->setFocus();
}

void KateNormalInputMode::findSelectedForwards()
{
    searchBar(IncrementalSearchBarOrKeepMode)->nextMatchForSelection(view(), KateSearchBar::SearchForward);
}

void KateNormalInputMode::findSelectedBackwards()
{
    searchBar(IncrementalSearchBarOrKeepMode)->nextMatchForSelection(view(), KateSearchBar::SearchBackward);
}

void KateNormalInputMode::findReplace()
{
    KateSearchBar *const bar = searchBar(PowerSearchBar);
    view()->bottomViewBar()->addBarWidget(bar);
    view()->bottomViewBar()->showBarWidget(bar);
    bar->setFocus();
}

void KateNormalInputMode::findNext()
{
    searchBar(IncrementalSearchBarOrKeepMode)->findNext();
}

void KateNormalInputMode::findPrevious()
{
    searchBar(IncrementalSearchBarOrKeepMode)->findPrevious();
}

void KateNormalInputMode::activateCommandLine()
{
    const KTextEditor::Range selection = view()->selectionRange();

    // if the user has selected text, insert the selection's range (start line to end line) in the
    // command line when opened
    if (selection.start().line() != -1 && selection.end().line() != -1) {
        cmdLineBar()->setText(QString::number(selection.start().line() + 1) + QLatin1Char(',') + QString::number(selection.end().line() + 1));
    }
    view()->bottomViewBar()->showBarWidget(cmdLineBar());
    cmdLineBar()->setFocus();
}

KateSearchBar *KateNormalInputMode::searchBar(const SearchBarMode mode)
{
    /**
     * power mode wanted?
     */
    const bool wantPowerMode = (mode == PowerSearchBar);

    /**
     * create search bar is not there? use right mode
     */
    if (!m_searchBar) {
        m_searchBar = new KateSearchBar(wantPowerMode, view(), KateViewConfig::global());
    }

    /**
     * else: switch mode if needed!
     */
    else if (mode != IncrementalSearchBarOrKeepMode) {
        if (wantPowerMode) {
            m_searchBar->enterPowerMode();
        } else {
            m_searchBar->enterIncrementalMode();
        }
    }

    return m_searchBar;
}

KateCommandLineBar *KateNormalInputMode::cmdLineBar()
{
    if (!m_cmdLine) {
        m_cmdLine = new KateCommandLineBar(view(), view()->bottomViewBar());
        view()->bottomViewBar()->addBarWidget(m_cmdLine);
    }

    return m_cmdLine;
}

void KateNormalInputMode::updateRendererConfig()
{
    if (m_searchBar) {
        m_searchBar->updateHighlightColors();
    }
}

bool KateNormalInputMode::keyPress(QKeyEvent *e)
{
    // Note: AND'ing with <Shift> is a quick hack to fix Key_Enter
    const int key = e->key() | (e->modifiers() & Qt::ShiftModifier);

    if (view()->isCompletionActive()) {
        if (key == Qt::Key_Enter || key == Qt::Key_Return) {
            view()->completionWidget()->execute();
            e->accept();
            return true;
        }
    }

    return false;
}

bool KateNormalInputMode::blinkCaret() const
{
    return true;
}

KateRenderer::caretStyles KateNormalInputMode::caretStyle() const
{
    return view()->isOverwriteMode() ? KateRenderer::Block : KateRenderer::Line;
}

void KateNormalInputMode::toggleInsert()
{
    view()->toggleInsert();
}

void KateNormalInputMode::launchInteractiveCommand(const QString &command)
{
    KateCommandLineBar *cmdLine = cmdLineBar();
    view()->bottomViewBar()->showBarWidget(cmdLine);
    cmdLine->setText(command, false);
}

QString KateNormalInputMode::bookmarkLabel(int) const
{
    return QString();
}

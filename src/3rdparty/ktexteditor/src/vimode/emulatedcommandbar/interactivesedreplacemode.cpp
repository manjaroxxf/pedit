/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2013-2016 Simon St James <kdedevel@etotheipiplusone.com>

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

#include "interactivesedreplacemode.h"

#include <QKeyEvent>
#include <QLabel>

using namespace KateVi;

InteractiveSedReplaceMode::InteractiveSedReplaceMode(EmulatedCommandBar *emulatedCommandBar, MatchHighlighter *matchHighlighter, InputModeManager *viInputModeManager, KTextEditor::ViewPrivate *view)
    : ActiveMode(emulatedCommandBar, matchHighlighter, viInputModeManager, view)
    , m_isActive(false)
{
    m_interactiveSedReplaceLabel = new QLabel();
    m_interactiveSedReplaceLabel->setObjectName(QStringLiteral("interactivesedreplace"));
}

void InteractiveSedReplaceMode::activate(QSharedPointer<SedReplace::InteractiveSedReplacer> interactiveSedReplace)
{
    Q_ASSERT_X(interactiveSedReplace->currentMatch().isValid(), "startInteractiveSearchAndReplace", "KateCommands shouldn't initiate an interactive sed replace with no initial match");

    m_isActive = true;
    m_interactiveSedReplacer = interactiveSedReplace;

    hideAllWidgetsExcept(m_interactiveSedReplaceLabel);
    m_interactiveSedReplaceLabel->show();
    updateInteractiveSedReplaceLabelText();

    updateMatchHighlight(interactiveSedReplace->currentMatch());
    moveCursorTo(interactiveSedReplace->currentMatch().start());
}

bool InteractiveSedReplaceMode::handleKeyPress(const QKeyEvent *keyEvent)
{
    // TODO - it would be better to use e.g. keyEvent->key() == Qt::Key_Y instead of keyEvent->text() == "y",
    // but this would require some slightly dicey changes to the "feed key press" code in order to make it work
    // with mappings and macros.
    if (keyEvent->text() == QLatin1String("y") || keyEvent->text() == QLatin1String("n")) {
        const KTextEditor::Cursor cursorPosIfFinalMatch = m_interactiveSedReplacer->currentMatch().start();
        if (keyEvent->text() == QLatin1String("y")) {
            m_interactiveSedReplacer->replaceCurrentMatch();
        } else {
            m_interactiveSedReplacer->skipCurrentMatch();
        }
        updateMatchHighlight(m_interactiveSedReplacer->currentMatch());
        updateInteractiveSedReplaceLabelText();
        moveCursorTo(m_interactiveSedReplacer->currentMatch().start());

        if (!m_interactiveSedReplacer->currentMatch().isValid()) {
            moveCursorTo(cursorPosIfFinalMatch);
            finishInteractiveSedReplace();
        }
        return true;
    } else if (keyEvent->text() == QLatin1String("l")) {
        m_interactiveSedReplacer->replaceCurrentMatch();
        finishInteractiveSedReplace();
        return true;
    } else if (keyEvent->text() == QLatin1String("q")) {
        finishInteractiveSedReplace();
        return true;
    } else if (keyEvent->text() == QLatin1String("a")) {
        m_interactiveSedReplacer->replaceAllRemaining();
        finishInteractiveSedReplace();
        return true;
    }
    return false;
}

void InteractiveSedReplaceMode::deactivate(bool wasAborted)
{
    Q_UNUSED(wasAborted);
    m_isActive = false;
    m_interactiveSedReplaceLabel->hide();
}

QWidget *InteractiveSedReplaceMode::label()
{
    return m_interactiveSedReplaceLabel;
}

void InteractiveSedReplaceMode::updateInteractiveSedReplaceLabelText()
{
    m_interactiveSedReplaceLabel->setText(m_interactiveSedReplacer->currentMatchReplacementConfirmationMessage() + QLatin1String(" (y/n/a/q/l)"));
}

void InteractiveSedReplaceMode::finishInteractiveSedReplace()
{
    deactivate(false);
    closeWithStatusMessage(m_interactiveSedReplacer->finalStatusReportMessage());
    m_interactiveSedReplacer.clear();
}

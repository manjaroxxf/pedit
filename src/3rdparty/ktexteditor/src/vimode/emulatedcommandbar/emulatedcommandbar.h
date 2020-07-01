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

#ifndef KATEVI_EMULATED_COMMAND_BAR_H
#define KATEVI_EMULATED_COMMAND_BAR_H

#include "kateviewhelpers.h"
#include <vimode/cmds.h>

#include "../searcher.h"
#include "activemode.h"
#include <ktexteditor/movingrange.h>
#include <ktexteditor/range.h>

namespace KTextEditor
{
class ViewPrivate;
class Command;
}

class QLabel;
class QLayout;

namespace KateVi
{
class MatchHighlighter;
class InteractiveSedReplaceMode;
class SearchMode;
class CommandMode;

/**
 * A KateViewBarWidget that attempts to emulate some of the features of Vim's own command bar,
 * including insertion of register contents via ctr-r<registername>; dismissal via
 * ctrl-c and ctrl-[; bi-directional incremental searching, with SmartCase; interactive sed-replace;
 * plus a few extensions such as completion from document and navigable sed search and sed replace history.
 */
class KTEXTEDITOR_EXPORT EmulatedCommandBar : public KateViewBarWidget
{
    Q_OBJECT

public:
    enum Mode { NoMode, SearchForward, SearchBackward, Command };
    explicit EmulatedCommandBar(KateViInputMode *viInputMode, InputModeManager *viInputModeManager, QWidget *parent = nullptr);
    ~EmulatedCommandBar() override;
    void init(Mode mode, const QString &initialText = QString());
    bool isActive();
    void setCommandResponseMessageTimeout(long commandResponseMessageTimeOutMS);
    bool handleKeyPress(const QKeyEvent *keyEvent);
    bool isSendingSyntheticSearchCompletedKeypress();

    void startInteractiveSearchAndReplace(QSharedPointer<SedReplace::InteractiveSedReplacer> interactiveSedReplace);
    QString executeCommand(const QString &commandToExecute);

    void setViInputModeManager(InputModeManager *viInputModeManager);

private:
    KateViInputMode *m_viInputMode;
    InputModeManager *m_viInputModeManager;
    bool m_isActive = false;
    bool m_wasAborted = true;
    Mode m_mode = NoMode;
    KTextEditor::ViewPrivate *m_view = nullptr;
    QLineEdit *m_edit = nullptr;

    QLabel *m_barTypeIndicator = nullptr;
    void showBarTypeIndicator(Mode mode);

    bool m_suspendEditEventFiltering = false;

    bool m_waitingForRegister = false;
    QLabel *m_waitingForRegisterIndicator;
    bool m_insertedTextShouldBeEscapedForSearchingAsLiteral = false;

    void hideAllWidgetsExcept(QWidget *widgetToKeepVisible);

    friend class ActiveMode;
    QScopedPointer<MatchHighlighter> m_matchHighligher;

    QScopedPointer<Completer> m_completer;

    QScopedPointer<InteractiveSedReplaceMode> m_interactiveSedReplaceMode;
    QScopedPointer<SearchMode> m_searchMode;
    QScopedPointer<CommandMode> m_commandMode;

    void switchToMode(ActiveMode *newMode);
    ActiveMode *m_currentMode = nullptr;

    bool barHandledKeypress(const QKeyEvent *keyEvent);
    void insertRegisterContents(const QKeyEvent *keyEvent);
    bool eventFilter(QObject *object, QEvent *event) override;
    void deleteSpacesToLeftOfCursor();
    void deleteWordCharsToLeftOfCursor();
    bool deleteNonWordCharsToLeftOfCursor();

    void closed() override;
    void closeWithStatusMessage(const QString &exitStatusMessage);
    QTimer *m_exitStatusMessageDisplayHideTimer;
    QLabel *m_exitStatusMessageDisplay;
    long m_exitStatusMessageHideTimeOutMS = 4000;

    void createAndAddBarTypeIndicator(QLayout *layout);
    void createAndAddEditWidget(QLayout *layout);
    void createAndAddExitStatusMessageDisplay(QLayout *layout);
    void createAndInitExitStatusMessageDisplayTimer();
    void createAndAddWaitingForRegisterIndicator(QLayout *layout);

private Q_SLOTS:
    void editTextChanged(const QString &newText);
    void startHideExitStatusMessageTimer();
};

}

#endif /* KATEVI_EMULATED_COMMAND_BAR_H */

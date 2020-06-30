/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2008-2011 Erlend Hamberg <ehamberg@gmail.com>
    Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
    Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>

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

#include "katecompletiontree.h"
#include "katecompletionwidget.h"
#include "kateconfig.h"
#include "kateglobal.h"
#include "katepartdebug.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include "kateviinputmode.h"
#include <vimode/completionrecorder.h>
#include <vimode/completionreplayer.h>
#include <vimode/inputmodemanager.h>
#include <vimode/keyparser.h>
#include <vimode/lastchangerecorder.h>
#include <vimode/macrorecorder.h>
#include <vimode/marks.h>
#include <vimode/modes/insertvimode.h>

#include <KLocalizedString>

using namespace KateVi;

InsertViMode::InsertViMode(InputModeManager *viInputModeManager, KTextEditor::ViewPrivate *view, KateViewInternal *viewInternal)
    : ModeBase()
{
    m_view = view;
    m_viewInternal = viewInternal;
    m_viInputModeManager = viInputModeManager;

    m_waitingRegister = false;
    m_blockInsert = None;
    m_eolPos = 0;
    m_count = 1;
    m_countedRepeatsBeginOnNewLine = false;

    m_isExecutingCompletion = false;

    connect(doc(), SIGNAL(textInserted(KTextEditor::Document *, KTextEditor::Range)), this, SLOT(textInserted(KTextEditor::Document *, KTextEditor::Range)));
}

InsertViMode::~InsertViMode()
{
}

bool InsertViMode::commandInsertFromAbove()
{
    KTextEditor::Cursor c(m_view->cursorPosition());

    if (c.line() <= 0) {
        return false;
    }

    QString line = doc()->line(c.line() - 1);
    int tabWidth = doc()->config()->tabWidth();
    QChar ch = getCharAtVirtualColumn(line, m_view->virtualCursorColumn(), tabWidth);

    if (ch == QChar::Null) {
        return false;
    }

    return doc()->insertText(c, ch);
}

bool InsertViMode::commandInsertFromBelow()
{
    KTextEditor::Cursor c(m_view->cursorPosition());

    if (c.line() >= doc()->lines() - 1) {
        return false;
    }

    QString line = doc()->line(c.line() + 1);
    int tabWidth = doc()->config()->tabWidth();
    QChar ch = getCharAtVirtualColumn(line, m_view->virtualCursorColumn(), tabWidth);

    if (ch == QChar::Null) {
        return false;
    }

    return doc()->insertText(c, ch);
}

bool InsertViMode::commandDeleteWord()
{
    KTextEditor::Cursor c1(m_view->cursorPosition());
    KTextEditor::Cursor c2;

    c2 = findPrevWordStart(c1.line(), c1.column());

    if (c2.line() != c1.line()) {
        if (c1.column() == 0) {
            c2.setColumn(doc()->line(c2.line()).length());
        } else {
            c2.setColumn(0);
            c2.setLine(c2.line() + 1);
        }
    }

    Range r(c2, c1, ExclusiveMotion);
    return deleteRange(r, CharWise, false);
}

bool InsertViMode::commandDeleteLine()
{
    KTextEditor::Cursor c(m_view->cursorPosition());
    Range r(c.line(), 0, c.line(), c.column(), ExclusiveMotion);

    if (c.column() == 0) {
        // Try to move the current line to the end of the previous line.
        if (c.line() == 0) {
            return true;
        } else {
            r.startColumn = doc()->line(c.line() - 1).length();
            r.startLine--;
        }
    } else {
        /*
         * Remove backwards until the first non-space character. If no
         * non-space was found, remove backwards to the first column.
         */
        QRegExp nonSpace(QLatin1String("\\S"));
        r.startColumn = getLine().indexOf(nonSpace);
        if (r.startColumn == -1 || r.startColumn >= c.column()) {
            r.startColumn = 0;
        }
    }
    return deleteRange(r, CharWise, false);
}

bool InsertViMode::commandDeleteCharBackward()
{
    KTextEditor::Cursor c(m_view->cursorPosition());

    Range r(c.line(), c.column() - getCount(), c.line(), c.column(), ExclusiveMotion);

    if (c.column() == 0) {
        if (c.line() == 0) {
            return true;
        } else {
            r.startColumn = doc()->line(c.line() - 1).length();
            r.startLine--;
        }
    }

    return deleteRange(r, CharWise);
}

bool InsertViMode::commandNewLine()
{
    doc()->newLine(m_view);
    return true;
}

bool InsertViMode::commandIndent()
{
    KTextEditor::Cursor c(m_view->cursorPosition());
    doc()->indent(KTextEditor::Range(c.line(), 0, c.line(), 0), 1);
    return true;
}

bool InsertViMode::commandUnindent()
{
    KTextEditor::Cursor c(m_view->cursorPosition());
    doc()->indent(KTextEditor::Range(c.line(), 0, c.line(), 0), -1);
    return true;
}

bool InsertViMode::commandToFirstCharacterInFile()
{
    KTextEditor::Cursor c(0, 0);
    updateCursor(c);
    return true;
}

bool InsertViMode::commandToLastCharacterInFile()
{
    int lines = doc()->lines() - 1;
    KTextEditor::Cursor c(lines, doc()->line(lines).length());
    updateCursor(c);
    return true;
}

bool InsertViMode::commandMoveOneWordLeft()
{
    KTextEditor::Cursor c(m_view->cursorPosition());
    c = findPrevWordStart(c.line(), c.column());

    if (!c.isValid()) {
        c = KTextEditor::Cursor(0, 0);
    }

    updateCursor(c);
    return true;
}

bool InsertViMode::commandMoveOneWordRight()
{
    KTextEditor::Cursor c(m_view->cursorPosition());
    c = findNextWordStart(c.line(), c.column());

    if (!c.isValid()) {
        c = doc()->documentEnd();
    }

    updateCursor(c);
    return true;
}

bool InsertViMode::commandCompleteNext()
{
    if (m_view->completionWidget()->isCompletionActive()) {
        const QModelIndex oldCompletionItem = m_view->completionWidget()->treeView()->selectionModel()->currentIndex();
        m_view->completionWidget()->cursorDown();
        const QModelIndex newCompletionItem = m_view->completionWidget()->treeView()->selectionModel()->currentIndex();
        if (newCompletionItem == oldCompletionItem) {
            // Wrap to top.
            m_view->completionWidget()->top();
        }
    } else {
        m_view->userInvokedCompletion();
    }
    return true;
}

bool InsertViMode::commandCompletePrevious()
{
    if (m_view->completionWidget()->isCompletionActive()) {
        const QModelIndex oldCompletionItem = m_view->completionWidget()->treeView()->selectionModel()->currentIndex();
        m_view->completionWidget()->cursorUp();
        const QModelIndex newCompletionItem = m_view->completionWidget()->treeView()->selectionModel()->currentIndex();
        if (newCompletionItem == oldCompletionItem) {
            // Wrap to bottom.
            m_view->completionWidget()->bottom();
        }
    } else {
        m_view->userInvokedCompletion();
        m_view->completionWidget()->bottom();
    }
    return true;
}

bool InsertViMode::commandInsertContentOfRegister()
{
    KTextEditor::Cursor c(m_view->cursorPosition());
    KTextEditor::Cursor cAfter = c;
    QChar reg = getChosenRegister(m_register);

    OperationMode m = getRegisterFlag(reg);
    QString textToInsert = getRegisterContent(reg);

    if (textToInsert.isNull()) {
        error(i18n("Nothing in register %1", reg));
        return false;
    }

    if (m == LineWise) {
        textToInsert.chop(1);                     // remove the last \n
        c.setColumn(doc()->lineLength(c.line())); // paste after the current line and ...
        textToInsert.prepend(QLatin1Char('\n'));  // ... prepend a \n, so the text starts on a new line

        cAfter.setLine(cAfter.line() + 1);
        cAfter.setColumn(0);
    } else {
        cAfter.setColumn(cAfter.column() + textToInsert.length());
    }

    doc()->insertText(c, textToInsert, m == Block);

    updateCursor(cAfter);

    return true;
}

// Start Normal mode just for one command and return to Insert mode
bool InsertViMode::commandSwitchToNormalModeForJustOneCommand()
{
    m_viInputModeManager->setTemporaryNormalMode(true);
    m_viInputModeManager->changeViMode(ViMode::NormalMode);
    const KTextEditor::Cursor cursorPos = m_view->cursorPosition();
    // If we're at end of the line, move the cursor back one step, as in Vim.
    if (doc()->line(cursorPos.line()).length() == cursorPos.column()) {
        m_view->setCursorPosition(KTextEditor::Cursor(cursorPos.line(), cursorPos.column() - 1));
    }
    m_viInputModeManager->inputAdapter()->setCaretStyle(KateRenderer::Block);
    emit m_view->viewModeChanged(m_view, m_view->viewMode());
    m_viewInternal->repaint();
    return true;
}

/**
 * checks if the key is a valid command
 * @return true if a command was completed and executed, false otherwise
 */
bool InsertViMode::handleKeypress(const QKeyEvent *e)
{
    // backspace should work even if the shift key is down
    if (e->modifiers() != Qt::ControlModifier && e->key() == Qt::Key_Backspace) {
        m_view->backspace();
        return true;
    }

    if (m_keys.isEmpty() && !m_waitingRegister) {
        if (e->modifiers() == Qt::NoModifier) {
            switch (e->key()) {
            case Qt::Key_Escape:
                leaveInsertMode();
                return true;
            case Qt::Key_Left:
                m_view->cursorLeft();
                return true;
            case Qt::Key_Right:
                m_view->cursorRight();
                return true;
            case Qt::Key_Up:
                m_view->up();
                return true;
            case Qt::Key_Down:
                m_view->down();
                return true;
            case Qt::Key_Insert:
                startReplaceMode();
                return true;
            case Qt::Key_Delete:
                m_view->keyDelete();
                return true;
            case Qt::Key_Home:
                m_view->home();
                return true;
            case Qt::Key_End:
                m_view->end();
                return true;
            case Qt::Key_PageUp:
                m_view->pageUp();
                return true;
            case Qt::Key_PageDown:
                m_view->pageDown();
                return true;
            case Qt::Key_Enter:
            case Qt::Key_Return:
                if (m_view->completionWidget()->isCompletionActive() && !m_viInputModeManager->macroRecorder()->isReplaying() && !m_viInputModeManager->lastChangeRecorder()->isReplaying()) {
                    // Filter out Enter/ Return's that trigger a completion when recording macros/ last change stuff; they
                    // will be replaced with the special code "ctrl-space".
                    // (This is why there is a "!m_viInputModeManager->isReplayingMacro()" above.)
                    m_viInputModeManager->doNotLogCurrentKeypress();

                    m_isExecutingCompletion = true;
                    m_textInsertedByCompletion.clear();
                    m_view->completionWidget()->execute();
                    completionFinished();
                    m_isExecutingCompletion = false;
                    return true;
                }
                Q_FALLTHROUGH();
            default:
                return false;
            }
        } else if (e->modifiers() == Qt::ControlModifier) {
            switch (e->key()) {
            case Qt::Key_BracketLeft:
            case Qt::Key_3:
                leaveInsertMode();
                return true;
            case Qt::Key_Space:
                // We use Ctrl-space as a special code in macros/ last change, which means: if replaying
                // a macro/ last change, fetch and execute the next completion for this macro/ last change ...
                if (!m_viInputModeManager->macroRecorder()->isReplaying() && !m_viInputModeManager->lastChangeRecorder()->isReplaying()) {
                    commandCompleteNext();
                    // ... therefore, we should not record ctrl-space indiscriminately.
                    m_viInputModeManager->doNotLogCurrentKeypress();
                } else {
                    m_viInputModeManager->completionReplayer()->replay();
                }
                return true;
            case Qt::Key_C:
                leaveInsertMode(true);
                return true;
            case Qt::Key_D:
                commandUnindent();
                return true;
            case Qt::Key_E:
                commandInsertFromBelow();
                return true;
            case Qt::Key_N:
                if (!m_viInputModeManager->macroRecorder()->isReplaying()) {
                    commandCompleteNext();
                }
                return true;
            case Qt::Key_P:
                if (!m_viInputModeManager->macroRecorder()->isReplaying()) {
                    commandCompletePrevious();
                }
                return true;
            case Qt::Key_T:
                commandIndent();
                return true;
            case Qt::Key_W:
                commandDeleteWord();
                return true;
            case Qt::Key_U:
                return commandDeleteLine();
            case Qt::Key_J:
                commandNewLine();
                return true;
            case Qt::Key_H:
                commandDeleteCharBackward();
                return true;
            case Qt::Key_Y:
                commandInsertFromAbove();
                return true;
            case Qt::Key_O:
                commandSwitchToNormalModeForJustOneCommand();
                return true;
            case Qt::Key_Home:
                commandToFirstCharacterInFile();
                return true;
            case Qt::Key_R:
                m_waitingRegister = true;
                return true;
            case Qt::Key_End:
                commandToLastCharacterInFile();
                return true;
            case Qt::Key_Left:
                commandMoveOneWordLeft();
                return true;
            case Qt::Key_Right:
                commandMoveOneWordRight();
                return true;
            default:
                return false;
            }
        }

        return false;
    } else if (m_waitingRegister) {
        // ignore modifier keys alone
        if (e->key() == Qt::Key_Shift || e->key() == Qt::Key_Control || e->key() == Qt::Key_Alt || e->key() == Qt::Key_Meta) {
            return false;
        }

        QChar key = KeyParser::self()->KeyEventToQChar(*e);
        key = key.toLower();
        m_waitingRegister = false;

        // is it register ?
        // TODO: add registers such as '/'. See :h <c-r>
        if ((key >= QLatin1Char('0') && key <= QLatin1Char('9')) || (key >= QLatin1Char('a') && key <= QLatin1Char('z')) || key == QLatin1Char('_') || key == QLatin1Char('+') || key == QLatin1Char('*') || key == QLatin1Char('"')) {
            m_register = key;
        } else {
            return false;
        }
        commandInsertContentOfRegister();
        return true;
    }
    return false;
}

// leave insert mode when esc, etc, is pressed. if leaving block
// prepend/append, the inserted text will be added to all block lines. if
// ctrl-c is used to exit insert mode this is not done.
void InsertViMode::leaveInsertMode(bool force)
{
    m_view->abortCompletion();
    if (!force) {
        if (m_blockInsert != None) { // block append/prepend

            // make sure cursor haven't been moved
            if (m_blockRange.startLine == m_view->cursorPosition().line()) {
                int start, len;
                QString added;
                KTextEditor::Cursor c;

                switch (m_blockInsert) {
                case Append:
                case Prepend:
                    if (m_blockInsert == Append) {
                        start = m_blockRange.endColumn + 1;
                    } else {
                        start = m_blockRange.startColumn;
                    }

                    len = m_view->cursorPosition().column() - start;
                    added = getLine().mid(start, len);

                    c = KTextEditor::Cursor(m_blockRange.startLine, start);
                    for (int i = m_blockRange.startLine + 1; i <= m_blockRange.endLine; i++) {
                        c.setLine(i);
                        doc()->insertText(c, added);
                    }
                    break;
                case AppendEOL:
                    start = m_eolPos;
                    len = m_view->cursorPosition().column() - start;
                    added = getLine().mid(start, len);

                    c = KTextEditor::Cursor(m_blockRange.startLine, start);
                    for (int i = m_blockRange.startLine + 1; i <= m_blockRange.endLine; i++) {
                        c.setLine(i);
                        c.setColumn(doc()->lineLength(i));
                        doc()->insertText(c, added);
                    }
                    break;
                default:
                    error(QStringLiteral("not supported"));
                }
            }

            m_blockInsert = None;
        } else {
            const QString added = doc()->text(KTextEditor::Range(m_viInputModeManager->marks()->getStartEditYanked(), m_view->cursorPosition()));

            if (m_count > 1) {
                for (unsigned int i = 0; i < m_count - 1; i++) {
                    if (m_countedRepeatsBeginOnNewLine) {
                        doc()->newLine(m_view);
                    }
                    doc()->insertText(m_view->cursorPosition(), added);
                }
            }
        }
    }
    m_countedRepeatsBeginOnNewLine = false;
    startNormalMode();
}

void InsertViMode::setBlockPrependMode(Range blockRange)
{
    // ignore if not more than one line is selected
    if (blockRange.startLine != blockRange.endLine) {
        m_blockInsert = Prepend;
        m_blockRange = blockRange;
    }
}

void InsertViMode::setBlockAppendMode(Range blockRange, BlockInsert b)
{
    Q_ASSERT(b == Append || b == AppendEOL);

    // ignore if not more than one line is selected
    if (blockRange.startLine != blockRange.endLine) {
        m_blockRange = blockRange;
        m_blockInsert = b;
        if (b == AppendEOL) {
            m_eolPos = doc()->lineLength(m_blockRange.startLine);
        }
    } else {
        qCDebug(LOG_KTE) << "cursor moved. ignoring block append/prepend";
    }
}

void InsertViMode::completionFinished()
{
    Completion::CompletionType completionType = Completion::PlainText;
    if (m_view->cursorPosition() != m_textInsertedByCompletionEndPos) {
        completionType = Completion::FunctionWithArgs;
    } else if (m_textInsertedByCompletion.endsWith(QLatin1String("()")) || m_textInsertedByCompletion.endsWith(QLatin1String("();"))) {
        completionType = Completion::FunctionWithoutArgs;
    }
    m_viInputModeManager->completionRecorder()->logCompletionEvent(Completion(m_textInsertedByCompletion, KateViewConfig::global()->wordCompletionRemoveTail(), completionType));
}

void InsertViMode::textInserted(KTextEditor::Document *document, KTextEditor::Range range)
{
    if (m_isExecutingCompletion) {
        m_textInsertedByCompletion += document->text(range);
        m_textInsertedByCompletionEndPos = range.end();
    }
}

/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2008 Niko Sams <niko.sams@gmail.com>

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

#include "codecompletionmodelcontrollerinterface.h"

#include <QModelIndex>
#include <QRegularExpression>

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

namespace KTextEditor
{
CodeCompletionModelControllerInterface::CodeCompletionModelControllerInterface()
{
}

CodeCompletionModelControllerInterface::~CodeCompletionModelControllerInterface()
{
}

bool CodeCompletionModelControllerInterface::shouldStartCompletion(View *view, const QString &insertedText, bool userInsertion, const Cursor &position)
{
    Q_UNUSED(view);
    Q_UNUSED(position);
    if (insertedText.isEmpty()) {
        return false;
    }

    QChar lastChar = insertedText.at(insertedText.count() - 1);
    if ((userInsertion && (lastChar.isLetter() || lastChar.isNumber() || lastChar == QLatin1Char('_'))) || lastChar == QLatin1Char('.') || insertedText.endsWith(QLatin1String("->"))) {
        return true;
    }
    return false;
}

Range CodeCompletionModelControllerInterface::completionRange(View *view, const Cursor &position)
{
    Cursor end = position;

    QString text = view->document()->line(end.line());

    static const QRegularExpression findWordStart(QStringLiteral("\\b[_\\w]+$"));
    static const QRegularExpression findWordEnd(QStringLiteral("^[_\\w]*\\b"));

    Cursor start = end;

    int pos = text.left(end.column()).lastIndexOf(findWordStart);
    if (pos >= 0) {
        start.setColumn(pos);
    }

    QRegularExpressionMatch match;
    pos = text.mid(end.column()).indexOf(findWordEnd, 0, &match);
    if (pos >= 0) {
        end.setColumn(end.column() + match.capturedLength());
    }

    return Range(start, end);
}

Range CodeCompletionModelControllerInterface::updateCompletionRange(View *view, const Range &range)
{
    QStringList text = view->document()->textLines(range, false);
    if (!text.isEmpty() && text.count() == 1 && text.first().trimmed().isEmpty())
    // When inserting a newline behind an empty completion-range,, move the range forward to its end
    {
        return Range(range.end(), range.end());
    }

    return range;
}

QString CodeCompletionModelControllerInterface::filterString(View *view, const Range &range, const Cursor &position)
{
    return view->document()->text(KTextEditor::Range(range.start(), position));
}

bool CodeCompletionModelControllerInterface::shouldAbortCompletion(View *view, const Range &range, const QString &currentCompletion)
{
    if (view->cursorPosition() < range.start() || view->cursorPosition() > range.end()) {
        return true; // Always abort when the completion-range has been left
    }
    // Do not abort completions when the text has been empty already before and a newline has been entered

    static const QRegularExpression allowedText(QStringLiteral("^\\w*$"));
    return !allowedText.match(currentCompletion).hasMatch();
}

void CodeCompletionModelControllerInterface::aborted(KTextEditor::View *view)
{
    Q_UNUSED(view);
}

bool CodeCompletionModelControllerInterface::shouldExecute(const QModelIndex &index, QChar inserted)
{
    Q_UNUSED(index);
    Q_UNUSED(inserted);
    return false;
}

KTextEditor::CodeCompletionModelControllerInterface::MatchReaction CodeCompletionModelControllerInterface::matchingItem(const QModelIndex &selected)
{
    Q_UNUSED(selected)
    return HideListIfAutomaticInvocation;
}

bool CodeCompletionModelControllerInterface::shouldHideItemsWithEqualNames() const
{
    return false;
}

}

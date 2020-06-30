/* This file is part of the KDE libraries
   Copyright (C) 2008 Niko Sams <niko.sams\gmail.com>

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

#ifndef KATE_COMPLETIONTESTMODELS_H
#define KATE_COMPLETIONTESTMODELS_H

#include "codecompletiontestmodel.h"
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QRegularExpression>

using namespace KTextEditor;

class CustomRangeModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    explicit CustomRangeModel(KTextEditor::View *parent = nullptr, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {
    }
    Range completionRange(View *view, const Cursor &position) override
    {
        Range range = CodeCompletionModelControllerInterface::completionRange(view, position);
        if (range.start().column() > 0) {
            KTextEditor::Range preRange(Cursor(range.start().line(), range.start().column() - 1), Cursor(range.start().line(), range.start().column()));
            qDebug() << preRange << view->document()->text(preRange);
            if (view->document()->text(preRange) == "$") {
                range.expandToRange(preRange);
                qDebug() << "using custom completion range" << range;
            }
        }
        return range;
    }

    bool shouldAbortCompletion(View *view, const Range &range, const QString &currentCompletion) override
    {
        Q_UNUSED(view);
        Q_UNUSED(range);
        static const QRegularExpression allowedText("^\\$?(\\w*)$");
        return !allowedText.match(currentCompletion).hasMatch();
    }
};

class CustomAbortModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    explicit CustomAbortModel(KTextEditor::View *parent = nullptr, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {
    }

    bool shouldAbortCompletion(View *view, const Range &range, const QString &currentCompletion) override
    {
        Q_UNUSED(view);
        Q_UNUSED(range);
        static const QRegularExpression allowedText("^([\\w-]*)");
        return !allowedText.match(currentCompletion).hasMatch();
    }
};

class EmptyFilterStringModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    explicit EmptyFilterStringModel(KTextEditor::View *parent = nullptr, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {
    }

    QString filterString(View *, const Range &, const Cursor &) override
    {
        return QString();
    }
};

class UpdateCompletionRangeModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    explicit UpdateCompletionRangeModel(KTextEditor::View *parent = nullptr, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {
    }

    Range updateCompletionRange(View *view, const Range &range) override
    {
        Q_UNUSED(view);
        if (view->document()->text(range) == QString("ab")) {
            return Range(Cursor(range.start().line(), 0), range.end());
        }
        return range;
    }
    bool shouldAbortCompletion(View *view, const Range &range, const QString &currentCompletion) override
    {
        Q_UNUSED(view);
        Q_UNUSED(range);
        Q_UNUSED(currentCompletion);
        return false;
    }
};

class StartCompletionModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    explicit StartCompletionModel(KTextEditor::View *parent = nullptr, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {
    }

    bool shouldStartCompletion(View *view, const QString &insertedText, bool userInsertion, const Cursor &position) override
    {
        Q_UNUSED(view);
        Q_UNUSED(userInsertion);
        Q_UNUSED(position);
        if (insertedText.isEmpty()) {
            return false;
        }

        QChar lastChar = insertedText.at(insertedText.count() - 1);
        if (lastChar == '%') {
            return true;
        }
        return false;
    }
};

class ImmideatelyAbortCompletionModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    explicit ImmideatelyAbortCompletionModel(KTextEditor::View *parent = nullptr, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {
    }

    bool shouldAbortCompletion(KTextEditor::View *view, const KTextEditor::Range &range, const QString &currentCompletion) override
    {
        Q_UNUSED(view);
        Q_UNUSED(range);
        Q_UNUSED(currentCompletion);
        return true;
    }
};

#endif

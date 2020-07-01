/* This file is part of the KDE libraries
   Copyright (C) 2012-2018 Dominik Haumann <dhaumann@kde.org>

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

#include "bug313769.h"
#include "moc_bug313769.cpp"

#include <katebuffer.h>
#include <kateconfig.h>
#include <katedocument.h>
#include <kateglobal.h>
#include <kateview.h>
#include <ktexteditor/range.h>

#include <QtTestWidgets>

QTEST_MAIN(BugTest)

using namespace KTextEditor;

BugTest::BugTest()
    : QObject()
{
}

BugTest::~BugTest()
{
}

void BugTest::initTestCase()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

void BugTest::cleanupTestCase()
{
}

void BugTest::tryCrash()
{
    KTextEditor::DocumentPrivate doc(false, false);
    const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR "bug313769.cpp"));
    doc.openUrl(url);
    doc.discardDataRecovery();
    doc.setHighlightingMode(QStringLiteral("C++"));
    doc.buffer().ensureHighlighted(doc.lines());

    // view must be visible...
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->show();
    view->resize(900, 800);
    view->config()->setDynWordWrap(true);
    view->setSelection(Range(2, 0, 74, 0));
    view->setCursorPosition(Cursor(74, 0));

    doc.editBegin();
    QString text = doc.line(1);
    doc.insertLine(74, text);
    doc.removeLine(1);
    view->setCursorPosition(Cursor(1, 0));
    doc.editEnd();

    QTest::qWait(200);
    // fold toplevel nodes
    for (int line = 0; line < doc.lines(); ++line) {
        if (view->textFolding().isLineVisible(line)) {
            view->foldLine(line);
        }
    }
    doc.buffer().ensureHighlighted(doc.lines());

    view->setCursorPosition(Cursor(0, 0));

    QTest::qWait(100);
    doc.undo();
    QTest::qWait(100);
    doc.redo();
    QTest::qWait(500);
    qDebug() << "!!! Does undo crash?";
    doc.undo();

    QTest::qWait(500);
    qDebug() << "!!! No crash (qWait not long enough)? Nice!";
}

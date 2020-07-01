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

#include "bug286887.h"

#include <kateconfig.h>
#include <katedocument.h>
#include <kateglobal.h>
#include <kateview.h>

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

void BugTest::ctrlShiftLeft()
{
    KTextEditor::DocumentPrivate doc(false, false);

    // view must be visible...
    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->show();
    view->resize(400, 300);

    // enable block mode, then set cursor after last character, then shift+left
    doc.clear();
    view->setBlockSelection(true);
    view->setCursorPosition(Cursor(0, 2));
    view->shiftCursorLeft();

    QTest::qWait(500);

    // enable block mode, then set cursor after last character, then delete word left
    doc.clear();
    view->setBlockSelection(true);
    view->setCursorPosition(Cursor(0, 2));
    view->deleteWordLeft();

    QTest::qWait(500);

    // disable wrap-cursor, then set cursor after last character, then shift+left
    doc.clear();
    view->setBlockSelection(false);
    view->setCursorPosition(Cursor(0, 2));
    view->shiftCursorLeft();

    QTest::qWait(500);

    // disable wrap-cursor, then set cursor after last character, then delete word left
    doc.clear();
    view->setCursorPosition(Cursor(0, 2));
    view->deleteWordLeft();
}

#include "moc_bug286887.cpp"

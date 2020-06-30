/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

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

#include "scriptdocument_test.h"

#include "ktexteditor/cursor.h"
#include <katedocument.h>
#include <kateglobal.h>
#include <katescriptdocument.h>
#include <ktexteditor/view.h>

#include <QJSEngine>
#include <QtTestWidgets>

QTEST_MAIN(ScriptDocumentTest)

QtMessageHandler ScriptDocumentTest::s_msgHandler = nullptr;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    switch (type) {
    case QtDebugMsg:
        /* do nothing */
        break;
    default:
        ScriptDocumentTest::s_msgHandler(type, context, msg);
    }
}

void ScriptDocumentTest::initTestCase()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
    s_msgHandler = qInstallMessageHandler(myMessageOutput);
}

void ScriptDocumentTest::cleanupTestCase()
{
    qInstallMessageHandler(nullptr);
}

ScriptDocumentTest::ScriptDocumentTest()
    : QObject()
{
}

ScriptDocumentTest::~ScriptDocumentTest()
{
}

void ScriptDocumentTest::init()
{
    m_doc = new KTextEditor::DocumentPrivate;
    m_view = m_doc->createView(nullptr);
    m_scriptDoc = new KateScriptDocument(nullptr, this);
    m_scriptDoc->setDocument(m_doc);
}

void ScriptDocumentTest::cleanup()
{
    delete m_scriptDoc;
    delete m_view;
    delete m_doc;
}

#if 0
void ScriptDocumentTest::testRfind_data()
{
    QTest::addColumn<KTextEditor::Range>("searchRange");
    QTest::addColumn<KTextEditor::Range>("expectedResult");

    QTest::newRow("") << KTextEditor::Range(0, 0, 1, 10) << KTextEditor::Range(1,  6, 1, 10);
    QTest::newRow("") << KTextEditor::Range(0, 0, 1,  5) << KTextEditor::Range(1,  0, 1,  4);
    QTest::newRow("") << KTextEditor::Range(0, 0, 1,  0) << KTextEditor::Range(0, 10, 0, 14);
}

void ScriptDocumentTest::testRfind()
{
    QFETCH(KTextEditor::Range, searchRange);
    QFETCH(KTextEditor::Range, expectedResult);

    m_doc->setText("aaaa aaaa aaaa\n"
                   "aaaa  aaaa");

    QCOMPARE(m_search->search(searchRange, "aaaa", true), expectedResult);
}
#endif

void ScriptDocumentTest::testRfind_data()
{
    QTest::addColumn<KTextEditor::Cursor>("searchStart");
    QTest::addColumn<KTextEditor::Cursor>("result");

    QTest::newRow("a a a a a a a a a a a a|") << KTextEditor::Cursor(0, 23) << KTextEditor::Cursor(0, 18);
    QTest::newRow("a a a a a a a a a a a |a") << KTextEditor::Cursor(0, 22) << KTextEditor::Cursor(0, 16);
    QTest::newRow("a a a a| a a a a a a a a") << KTextEditor::Cursor(0, 7) << KTextEditor::Cursor(0, 2);
    QTest::newRow("a a a |a a a a a a a a a") << KTextEditor::Cursor(0, 6) << KTextEditor::Cursor(0, 0);
    QTest::newRow("a a a| a a a a a a a a a") << KTextEditor::Cursor(0, 5) << KTextEditor::Cursor(0, 0);
    QTest::newRow("a a |a a a a a a a a a a") << KTextEditor::Cursor(0, 4) << KTextEditor::Cursor::invalid();
}

void ScriptDocumentTest::testRfind()
{
    QFETCH(KTextEditor::Cursor, searchStart);
    QFETCH(KTextEditor::Cursor, result);

    m_scriptDoc->setText("a a a a a a a a a a a a");

    KTextEditor::Cursor cursor = m_scriptDoc->rfind(searchStart, "a a a");
    QCOMPARE(cursor, result);
}

#include "moc_scriptdocument_test.cpp"

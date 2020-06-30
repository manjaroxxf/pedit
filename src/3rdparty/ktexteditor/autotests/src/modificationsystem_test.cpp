/* This file is part of the KDE libraries
   Copyright (C) 2011-2018 Dominik Haumann <dhaumann@kde.org>

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

#include "modificationsystem_test.h"
#include "moc_modificationsystem_test.cpp"

#include <katedocument.h>
#include <kateglobal.h>
#include <kateundomanager.h>

#include <QtTestWidgets>

QTEST_MAIN(ModificationSystemTest)

using namespace KTextEditor;

void ModificationSystemTest::initTestCase()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

void ModificationSystemTest::cleanupTestCase()
{
}

static void clearModificationFlags(KTextEditor::DocumentPrivate *doc)
{
    for (int i = 0; i < doc->lines(); ++i) {
        Kate::TextLine line = doc->plainKateTextLine(i);
        line->markAsModified(false);
        line->markAsSavedOnDisk(false);
    }
}

static void markModifiedLinesAsSaved(KTextEditor::DocumentPrivate *doc)
{
    for (int i = 0; i < doc->lines(); ++i) {
        Kate::TextLine textLine = doc->plainKateTextLine(i);
        if (textLine->markedAsModified()) {
            textLine->markAsSavedOnDisk(true);
        }
    }
}

void ModificationSystemTest::testInsertText()
{
    KTextEditor::DocumentPrivate doc;

    const QString content("first line\n");
    doc.setText(content);

    // now all lines should have state "Modified"
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    // now we have an a unmodified file, start real tests
    // insert text in line 0, then undo and redo
    doc.insertText(Cursor(0, 2), "_");
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineSaved(0));

    doc.redo();
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc.setModified(false);
    markModifiedLinesAsSaved(&doc);
    doc.undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineSaved(0));

    // undo the text insertion
    doc.undo();
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineSaved(0));
}

void ModificationSystemTest::testRemoveText()
{
    KTextEditor::DocumentPrivate doc;

    const QString content("first line\n");
    doc.setText(content);

    // now all lines should have state "Modified"
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    // now we have an a unmodified file, start real tests
    // remove text in line 0, then undo and redo
    doc.removeText(Range(Cursor(0, 1), Cursor(0, 2)));
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineSaved(0));

    doc.redo();
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc.setModified(false);
    markModifiedLinesAsSaved(&doc);
    doc.undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineSaved(0));

    // undo the text insertion
    doc.undo();
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineSaved(0));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineSaved(0));
}

void ModificationSystemTest::testInsertLine()
{
    KTextEditor::DocumentPrivate doc;

    const QString content(
        "0\n"
        "2");
    doc.setText(content);

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    // insert at line 1
    doc.insertLine(1, "1");

    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc.setModified(false);
    markModifiedLinesAsSaved(&doc);
    doc.undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    // undo the text insertion
    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));
}

void ModificationSystemTest::testRemoveLine()
{
    KTextEditor::DocumentPrivate doc;

    const QString content(
        "0\n"
        "1\n"
        "2");
    doc.setText(content);

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    // remove at line 1
    doc.removeLine(1);

    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc.setModified(false);
    markModifiedLinesAsSaved(&doc);
    doc.undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    // undo the text insertion
    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
}

void ModificationSystemTest::testWrapLineMid()
{
    for (int i = 0; i < 2; ++i) {
        bool insertNewLine = (i == 1);
        KTextEditor::DocumentPrivate doc;

        const QString content(
            "aaaa\n"
            "bbbb\n"
            "cccc");
        doc.setText(content);

        // clear all modification flags, forces no flags
        doc.setModified(false);
        doc.undoManager()->updateLineModifications();
        clearModificationFlags(&doc);

        // wrap line 1 at |: bb|bb
        doc.editWrapLine(1, 2, insertNewLine);

        QVERIFY(!doc.isLineModified(0));
        QVERIFY(doc.isLineModified(1));
        QVERIFY(doc.isLineModified(2));
        QVERIFY(!doc.isLineSaved(0));
        QVERIFY(!doc.isLineSaved(1));
        QVERIFY(!doc.isLineSaved(2));

        doc.undo();
        QVERIFY(!doc.isLineModified(0));
        QVERIFY(!doc.isLineModified(1));
        QVERIFY(!doc.isLineSaved(0));
        QVERIFY(doc.isLineSaved(1));

        doc.redo();
        QVERIFY(!doc.isLineModified(0));
        QVERIFY(doc.isLineModified(1));
        QVERIFY(doc.isLineModified(2));
        QVERIFY(!doc.isLineSaved(0));
        QVERIFY(!doc.isLineSaved(1));
        QVERIFY(!doc.isLineSaved(2));

        //
        // now simulate "save", then do the undo/redo tests again
        //
        doc.setModified(false);
        markModifiedLinesAsSaved(&doc);
        doc.undoManager()->updateLineModifications();

        // now no line should have state "Modified"
        QVERIFY(!doc.isLineModified(0));
        QVERIFY(!doc.isLineModified(1));
        QVERIFY(!doc.isLineModified(2));
        QVERIFY(!doc.isLineSaved(0));
        QVERIFY(doc.isLineSaved(1));
        QVERIFY(doc.isLineSaved(2));

        // undo the text insertion
        doc.undo();
        QVERIFY(!doc.isLineModified(0));
        QVERIFY(doc.isLineModified(1));
        QVERIFY(!doc.isLineSaved(0));
        QVERIFY(!doc.isLineSaved(1));

        doc.redo();
        QVERIFY(!doc.isLineModified(0));
        QVERIFY(!doc.isLineModified(1));
        QVERIFY(!doc.isLineModified(2));
        QVERIFY(!doc.isLineSaved(0));
        QVERIFY(doc.isLineSaved(1));
        QVERIFY(doc.isLineSaved(2));
    }
}

void ModificationSystemTest::testWrapLineAtEnd()
{
    KTextEditor::DocumentPrivate doc;

    const QString content(
        "aaaa\n"
        "bbbb");
    doc.setText(content);

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    // wrap line 0 at end
    doc.editWrapLine(0, 4);

    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc.setModified(false);
    markModifiedLinesAsSaved(&doc);
    doc.undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    // undo the text insertion
    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));
}

void ModificationSystemTest::testWrapLineAtStart()
{
    KTextEditor::DocumentPrivate doc;

    const QString content(
        "aaaa\n"
        "bbbb");
    doc.setText(content);

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    // wrap line 0 at end
    doc.editWrapLine(0, 0);

    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.redo();
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc.setModified(false);
    markModifiedLinesAsSaved(&doc);
    doc.undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    // undo the text insertion
    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));
}

void ModificationSystemTest::testUnWrapLine()
{
    KTextEditor::DocumentPrivate doc;

    const QString content(
        "aaaa\n"
        "bbbb\n"
        "cccc");
    doc.setText(content);

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    // join line 0 and 1
    doc.editUnWrapLine(0);

    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(doc.isLineSaved(0));
    QVERIFY(doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.redo();
    QVERIFY(doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc.setModified(false);
    markModifiedLinesAsSaved(&doc);
    doc.undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    // undo the text insertion
    doc.undo();
    QVERIFY(doc.isLineModified(0));
    QVERIFY(doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
}

void ModificationSystemTest::testUnWrapLine1Empty()
{
    KTextEditor::DocumentPrivate doc;

    const QString content(
        "aaaa\n"
        "\n"
        "bbbb");
    doc.setText(content);

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    // join line 1 and 2
    doc.editUnWrapLine(1);

    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc.setModified(false);
    markModifiedLinesAsSaved(&doc);
    doc.undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    // undo the text insertion
    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
}

void ModificationSystemTest::testUnWrapLine2Empty()
{
    KTextEditor::DocumentPrivate doc;

    const QString content(
        "aaaa\n"
        "\n"
        "bbbb");
    doc.setText(content);

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    // join line 0 and 1
    doc.editUnWrapLine(0);

    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc.setModified(false);
    markModifiedLinesAsSaved(&doc);
    doc.undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));

    // undo the text insertion
    doc.undo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(doc.isLineModified(1));
    QVERIFY(!doc.isLineModified(2));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
    QVERIFY(!doc.isLineSaved(2));

    doc.redo();
    QVERIFY(!doc.isLineModified(0));
    QVERIFY(!doc.isLineModified(1));
    QVERIFY(!doc.isLineSaved(0));
    QVERIFY(!doc.isLineSaved(1));
}

void ModificationSystemTest::testNavigation()
{
    KTextEditor::DocumentPrivate doc;

    const QString content(
        "0\n"
        "1\n"
        "2");
    doc.setText(content);

    // clear all modification flags, forces no flags
    doc.setModified(false);
    doc.undoManager()->updateLineModifications();
    clearModificationFlags(&doc);

    // touch line 0 and line 2:
    doc.insertText(Cursor(0, 1), QLatin1String("-"));
    doc.insertText(Cursor(2, 1), QLatin1String("-"));

    // test down navigation:
    const bool down = true;
    QCOMPARE(doc.findTouchedLine(-1, down), -1);
    QCOMPARE(doc.findTouchedLine(0, down), 0);
    QCOMPARE(doc.findTouchedLine(1, down), 2);
    QCOMPARE(doc.findTouchedLine(2, down), 2);
    QCOMPARE(doc.findTouchedLine(3, down), -1);

    // test up navigation
    const bool up = false;
    QCOMPARE(doc.findTouchedLine(-1, up), -1);
    QCOMPARE(doc.findTouchedLine(0, up), 0);
    QCOMPARE(doc.findTouchedLine(1, up), 0);
    QCOMPARE(doc.findTouchedLine(2, up), 2);
    QCOMPARE(doc.findTouchedLine(3, up), -1);
}

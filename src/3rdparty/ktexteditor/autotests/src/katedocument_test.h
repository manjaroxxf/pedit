/* This file is part of the KDE libraries
   Copyright (C) 2010-2018 Dominik Haumann <dhaumann@kde.org>

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

#ifndef KATE_DOCUMENT_TEST_H
#define KATE_DOCUMENT_TEST_H

#include <QObject>

class KateDocumentTest : public QObject
{
    Q_OBJECT

public:
    KateDocumentTest();
    ~KateDocumentTest();

public Q_SLOTS:
    void initTestCase();

private Q_SLOTS:
    void testWordWrap();
    void testWrapParagraph();
    void testReplaceQStringList();
    void testMovingInterfaceSignals();
    void testSetTextPerformance();
    void testRemoveTextPerformance();
    void testForgivingApiUsage();
    void testRemoveMultipleLines();
    void testInsertNewline();
    void testInsertAfterEOF();
    void testAutoBrackets();
    void testReplaceTabs();
    void testDigest();
    void testModelines();
    void testDefStyleNum();
    void testTypeCharsWithSurrogateAndNewLine();
    void testRemoveComposedCharacters();
    void testAutoReload();
    void testSearch();
};

#endif // KATE_DOCUMENT_TEST_H

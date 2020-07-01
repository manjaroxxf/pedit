/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#ifndef KTEXTEDITOR_VARIABLE_TEST_H
#define KTEXTEDITOR_VARIABLE_TEST_H

#include <QObject>

class VariableTest : public QObject
{
    Q_OBJECT

public:
    VariableTest();
    ~VariableTest();

private Q_SLOTS:
    void testReturnValues();
    void testExactMatch_data();
    void testExactMatch();
    void testPrefixMatch();
    void testRecursiveMatch();
    void testBuiltins();
};

#endif // KTEXTEDITOR_VARIABLE_TEST_H

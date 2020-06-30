/*
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

#ifndef SCRIPT_TEST_H
#define SCRIPT_TEST_H

#include <QObject>
#include <QPair>
#include <QStringList>

class TestScriptEnv;
namespace KTextEditor
{
class DocumentPrivate;
}
namespace KTextEditor
{
class ViewPrivate;
}
class QMainWindow;

class ScriptTestBase : public QObject
{
    Q_OBJECT

protected:
    void initTestCase();
    void cleanupTestCase();
    typedef QPair<const char *, const char *> Failure;
    typedef QList<Failure> ExpectedFailures;
    void getTestData(const QString &script);
    void runTest(const ExpectedFailures &failures);
    QByteArray digestForFile(const QString &file);

    TestScriptEnv *m_env;
    KTextEditor::DocumentPrivate *m_document;
    QMainWindow *m_toplevel;
    bool m_outputWasCustomised;
    QStringList m_commands;
    KTextEditor::ViewPrivate *m_view;
    QString m_section;    // dir name in testdata/
    QString m_script_dir; // dir name in part/script/data/

public:
    static QtMessageHandler m_msgHandler;
};

#endif // SCRIPT_TEST_H

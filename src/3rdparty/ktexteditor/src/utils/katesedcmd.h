/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2003-2005 Anders Lund <anders@alweb.dk>
    Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
    Copyright (C) 2001 Charles Samuels <charles@kde.org>

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

#ifndef KATE_SED_CMD_H
#define KATE_SED_CMD_H

#include "kateregexpsearch.h"
#include <KTextEditor/Command>

#include <QSharedPointer>
#include <QStringList>

namespace KTextEditor
{
class DocumentPrivate;
class ViewPrivate;
}

/**
 * The KateCommands namespace collects subclasses of KTextEditor::Command
 * for specific use in kate.
 */
namespace KateCommands
{
/**
 * Support vim/sed style search and replace
 * @author Charles Samuels <charles@kde.org>
 **/
class SedReplace : public KTextEditor::Command
{
    static SedReplace *m_instance;

protected:
    SedReplace()
        : KTextEditor::Command({QStringLiteral("s"), QStringLiteral("%s"), QStringLiteral("$s")})
    {
    }

public:
    ~SedReplace() override
    {
        m_instance = nullptr;
    }

    /**
     * Execute command. Valid command strings are:
     *   -  s/search/replace/  find @c search, replace it with @c replace
     *                         on this line
     *   -  \%s/search/replace/ do the same to the whole file
     *   -  s/search/replace/i do the search and replace case insensitively
     *   -  $s/search/replace/ do the search are replacement to the
     *                         selection only
     *
     * @note   $s/// is currently unsupported
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec(class KTextEditor::View *view, const QString &cmd, QString &errorMsg, const KTextEditor::Range &r) override;

    bool supportsRange(const QString &) override
    {
        return true;
    }

    /** This command does not have help. @see KTextEditor::Command::help */
    bool help(class KTextEditor::View *, const QString &, QString &) override
    {
        return false;
    }

    static SedReplace *self()
    {
        if (m_instance == nullptr) {
            m_instance = new SedReplace();
        }
        return m_instance;
    }

    /**
     * Parses @c sedReplaceString to see if it is a valid sed replace expression (e.g. "s/find/replace/gi").
     * If it is, returns true and sets @c delimiter to the delimiter used in the expression;
     *  @c destFindBeginPos and @c destFindEndPos to the positions in * @c sedReplaceString of the
     * begin and end of the "find" term; and @c destReplaceBeginPos and @c destReplaceEndPos to the begin
     * and end positions of the "replace" term.
     */
    static bool parse(const QString &sedReplaceString, QString &destDelim, int &destFindBeginPos, int &destFindEndPos, int &destReplaceBeginPos, int &destReplaceEndPos);

    class InteractiveSedReplacer
    {
    public:
        InteractiveSedReplacer(KTextEditor::DocumentPrivate *doc, const QString &findPattern, const QString &replacePattern, bool caseSensitive, bool onlyOnePerLine, int startLine, int endLine);
        /**
         * Will return invalid Range if there are no further matches.
         */
        KTextEditor::Range currentMatch();
        void skipCurrentMatch();
        void replaceCurrentMatch();
        void replaceAllRemaining();
        QString currentMatchReplacementConfirmationMessage();
        QString finalStatusReportMessage();

    private:
        const QString m_findPattern;
        const QString m_replacePattern;
        bool m_onlyOnePerLine;
        int m_endLine;
        KTextEditor::DocumentPrivate *m_doc;
        KateRegExpSearch m_regExpSearch;

        int m_numReplacementsDone;
        int m_numLinesTouched;
        int m_lastChangedLineNum;

        KTextEditor::Cursor m_currentSearchPos;
        const QVector<KTextEditor::Range> fullCurrentMatch();
        QString replacementTextForCurrentMatch();
    };

protected:
    virtual bool interactiveSedReplace(KTextEditor::ViewPrivate *kateView, QSharedPointer<InteractiveSedReplacer> interactiveSedReplace);
};

} // namespace KateCommands
#endif

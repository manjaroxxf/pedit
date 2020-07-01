/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
    Copyright (C) 2009-2018 Dominik Haumann <dhaumann@kde.org>

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

#include "kateindentscript.h"

#include <QJSEngine>
#include <QJSValue>

#include "katedocument.h"
#include "kateview.h"

KateIndentScript::KateIndentScript(const QString &url, const KateIndentScriptHeader &header)
    : KateScript(url)
    , m_indentHeader(header)
{
}

const KateIndentScriptHeader &KateIndentScript::indentHeader() const
{
    return m_indentHeader;
}

const QString &KateIndentScript::triggerCharacters()
{
    // already set, perfect, just return...
    if (m_triggerCharactersSet) {
        return m_triggerCharacters;
    }

    m_triggerCharactersSet = true;

    auto triggerCharacters = global(QStringLiteral("triggerCharacters"));
    if (!triggerCharacters.isUndefined()) {
        m_triggerCharacters = triggerCharacters.toString();
    }

    // qCDebug(LOG_KTE) << "trigger chars: '" << m_triggerCharacters << "'";

    return m_triggerCharacters;
}

QPair<int, int> KateIndentScript::indent(KTextEditor::ViewPrivate *view, const KTextEditor::Cursor &position, QChar typedCharacter, int indentWidth)
{
    // if it hasn't loaded or we can't load, return
    if (!setView(view)) {
        return qMakePair(-2, -2);
    }

    clearExceptions();
    QJSValue indentFunction = function(QStringLiteral("indent"));
    if (!indentFunction.isCallable()) {
        return qMakePair(-2, -2);
    }
    // add the arguments that we are going to pass to the function
    QJSValueList arguments;
    arguments << QJSValue(position.line());
    arguments << QJSValue(indentWidth);
    arguments << (typedCharacter.isNull() ? QJSValue(QString()) : QJSValue(QString(typedCharacter)));
    // get the required indent
    QJSValue result = indentFunction.call(arguments);
    // error during the calling?
    if (result.isError()) {
        displayBacktrace(result, QStringLiteral("Error calling indent()"));
        return qMakePair(-2, -2);
    }
    int indentAmount = -2;
    int alignAmount = -2;
    if (result.isArray()) {
        indentAmount = result.property(0).toInt();
        alignAmount = result.property(1).toInt();
    } else {
        indentAmount = result.toInt();
    }

    return qMakePair(indentAmount, alignAmount);
}

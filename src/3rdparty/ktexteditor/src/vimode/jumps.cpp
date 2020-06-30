/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) KDE Developers

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

#include "jumps.h"

using namespace KateVi;

void Jumps::add(const KTextEditor::Cursor &cursor)
{
    for (auto iterator = m_jumps.begin(); iterator != m_jumps.end(); iterator++) {
        if ((*iterator).line() == cursor.line()) {
            m_jumps.erase(iterator);
            break;
        }
    }

    m_jumps.push_back(cursor);
    m_current = m_jumps.end();
}

KTextEditor::Cursor Jumps::next(const KTextEditor::Cursor &cursor)
{
    if (m_current == m_jumps.end()) {
        return cursor;
    }

    KTextEditor::Cursor jump;
    if (m_current + 1 != m_jumps.end()) {
        jump = *(++m_current);
    } else {
        jump = *(m_current);
    }

    return jump;
}

KTextEditor::Cursor Jumps::prev(const KTextEditor::Cursor &cursor)
{
    if (m_current == m_jumps.end()) {
        add(cursor);
        m_current--;
    }

    if (m_current != m_jumps.begin()) {
        m_current--;
        return *m_current;
    }

    return cursor;
}

void Jumps::readSessionConfig(const KConfigGroup &config)
{
    // Format: jump1.line, jump1.column, jump2.line, jump2.column, jump3.line, ...
    m_jumps.clear();
    QStringList jumps = config.readEntry("JumpList", QStringList());

    for (int i = 0; i + 1 < jumps.size(); i += 2) {
        KTextEditor::Cursor jump = {jumps.at(i).toInt(), jumps.at(i + 1).toInt()};
        m_jumps.push_back(jump);
    }

    m_current = m_jumps.end();
}

void Jumps::writeSessionConfig(KConfigGroup &config) const
{
    // Format: jump1.line, jump1.column, jump2.line, jump2.column, jump3.line, ...
    QStringList l;
    for (const auto &jump : m_jumps) {
        l << QString::number(jump.line()) << QString::number(jump.column());
    }
    config.writeEntry("JumpList", l);
}

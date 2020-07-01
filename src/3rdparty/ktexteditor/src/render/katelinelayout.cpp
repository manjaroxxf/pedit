/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2002-2005 Hamish Rodda <rodda@kde.org>
    Copyright (C) 2003      Anakim Border <aborder@sources.sourceforge.net>

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

#include "katelinelayout.h"
#include "katetextfolding.h"
#include "katetextlayout.h"

#include <QTextLine>

#include "katepartdebug.h"

#include "katedocument.h"
#include "katerenderer.h"

KateLineLayout::KateLineLayout(KateRenderer &renderer)
    : m_renderer(renderer)
    , m_textLine()
    , m_line(-1)
    , m_virtualLine(-1)
    , m_shiftX(0)
    , m_layout(nullptr)
    , m_layoutDirty(true)
    , m_usePlainTextLine(false)
{
}

KateLineLayout::~KateLineLayout()
{
    delete m_layout;
}

void KateLineLayout::clear()
{
    m_textLine = Kate::TextLine();
    m_line = -1;
    m_virtualLine = -1;
    m_shiftX = 0;
    // not touching dirty
    delete m_layout;
    m_layout = nullptr;
    // not touching layout dirty
}

bool KateLineLayout::includesCursor(const KTextEditor::Cursor &realCursor) const
{
    return realCursor.line() == line();
}

const Kate::TextLine &KateLineLayout::textLine(bool reloadForce) const
{
    if (reloadForce || !m_textLine) {
        m_textLine = usePlainTextLine() ? m_renderer.doc()->plainKateTextLine(line()) : m_renderer.doc()->kateTextLine(line());
    }

    Q_ASSERT(m_textLine);

    return m_textLine;
}

int KateLineLayout::line() const
{
    return m_line;
}

void KateLineLayout::setLine(int line, int virtualLine)
{
    m_line = line;
    m_virtualLine = (virtualLine == -1) ? m_renderer.folding().lineToVisibleLine(line) : virtualLine;
    m_textLine = Kate::TextLine();
}

int KateLineLayout::virtualLine() const
{
    return m_virtualLine;
}

void KateLineLayout::setVirtualLine(int virtualLine)
{
    m_virtualLine = virtualLine;
}

bool KateLineLayout::startsInvisibleBlock() const
{
    if (!isValid()) {
        return false;
    }

    return (virtualLine() + 1) != m_renderer.folding().lineToVisibleLine(line() + 1);
}

int KateLineLayout::shiftX() const
{
    return m_shiftX;
}

void KateLineLayout::setShiftX(int shiftX)
{
    m_shiftX = shiftX;
}

KTextEditor::DocumentPrivate *KateLineLayout::doc() const
{
    return m_renderer.doc();
}

bool KateLineLayout::isValid() const
{
    return line() != -1 && layout() && textLine();
}

QTextLayout *KateLineLayout::layout() const
{
    return m_layout;
}

void KateLineLayout::setLayout(QTextLayout *layout)
{
    if (m_layout != layout) {
        delete m_layout;
        m_layout = layout;
    }

    m_layoutDirty = !m_layout;
    m_dirtyList.clear();
    if (m_layout)
        for (int i = 0; i < qMax(1, m_layout->lineCount()); ++i) {
            m_dirtyList.append(true);
        }
}

void KateLineLayout::invalidateLayout()
{
    setLayout(nullptr);
}

bool KateLineLayout::isDirty(int viewLine) const
{
    Q_ASSERT(isValid() && viewLine >= 0 && viewLine < viewLineCount());
    return m_dirtyList[viewLine];
}

bool KateLineLayout::setDirty(int viewLine, bool dirty)
{
    Q_ASSERT(isValid() && viewLine >= 0 && viewLine < viewLineCount());
    m_dirtyList[viewLine] = dirty;
    return dirty;
}

KTextEditor::Cursor KateLineLayout::start() const
{
    return KTextEditor::Cursor(line(), 0);
}

int KateLineLayout::length() const
{
    return textLine()->length();
}

int KateLineLayout::viewLineCount() const
{
    return m_layout->lineCount();
}

KateTextLayout KateLineLayout::viewLine(int viewLine) const
{
    if (viewLine < 0) {
        viewLine += viewLineCount();
    }
    Q_ASSERT(isValid());
    Q_ASSERT(viewLine >= 0 && viewLine < viewLineCount());
    return KateTextLayout(KateLineLayoutPtr(const_cast<KateLineLayout *>(this)), viewLine);
}

int KateLineLayout::width() const
{
    int width = 0;

    for (int i = 0; i < m_layout->lineCount(); ++i) {
        width = qMax((int)m_layout->lineAt(i).naturalTextWidth(), width);
    }

    return width;
}

int KateLineLayout::widthOfLastLine() const
{
    const KateTextLayout &lastLine = viewLine(viewLineCount() - 1);
    return lastLine.width() + lastLine.xOffset();
}

bool KateLineLayout::isOutsideDocument() const
{
    return line() < 0 || line() >= m_renderer.doc()->lines();
}

void KateLineLayout::debugOutput() const
{
    qCDebug(LOG_KTE) << "KateLineLayout: " << this << " valid " << isValid() << " line " << line() << " length " << length() << " width " << width() << " viewLineCount " << viewLineCount();
}

int KateLineLayout::viewLineForColumn(int column) const
{
    int len = 0;
    int i = 0;
    for (; i < m_layout->lineCount() - 1; ++i) {
        len += m_layout->lineAt(i).textLength();
        if (column < len) {
            return i;
        }
    }
    return i;
}

bool KateLineLayout::isLayoutDirty() const
{
    return m_layoutDirty;
}

void KateLineLayout::setLayoutDirty(bool dirty)
{
    m_layoutDirty = dirty;
}

bool KateLineLayout::usePlainTextLine() const
{
    return m_usePlainTextLine;
}

void KateLineLayout::setUsePlainTextLine(bool plain)
{
    m_usePlainTextLine = plain;
}

bool KateLineLayout::isRightToLeft() const
{
    if (!m_layout) {
        return false;
    }

    return m_layout->textOption().textDirection() == Qt::RightToLeft;
}

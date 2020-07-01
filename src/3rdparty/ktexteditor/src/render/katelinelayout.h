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

#ifndef _KATE_LINELAYOUT_H_
#define _KATE_LINELAYOUT_H_

#include <QExplicitlySharedDataPointer>
#include <QSharedData>

#include "katetextline.h"

#include <ktexteditor/cursor.h>

class QTextLayout;
namespace KTextEditor
{
class DocumentPrivate;
}
class KateTextLayout;
class KateRenderer;

class KateLineLayout : public QSharedData
{
public:
    explicit KateLineLayout(KateRenderer &renderer);
    ~KateLineLayout();

    KTextEditor::DocumentPrivate *doc() const;
    void debugOutput() const;

    void clear();
    bool isValid() const;
    bool isOutsideDocument() const;

    bool isRightToLeft() const;

    bool includesCursor(const KTextEditor::Cursor &realCursor) const;

    friend bool operator>(const KateLineLayout &r, const KTextEditor::Cursor &c);
    friend bool operator>=(const KateLineLayout &r, const KTextEditor::Cursor &c);
    friend bool operator<(const KateLineLayout &r, const KTextEditor::Cursor &c);
    friend bool operator<=(const KateLineLayout &r, const KTextEditor::Cursor &c);

    const Kate::TextLine &textLine(bool forceReload = false) const;
    int length() const;

    int line() const;
    /**
     * Only pass virtualLine if you know it (and thus we shouldn't try to look it up)
     */
    void setLine(int line, int virtualLine = -1);
    KTextEditor::Cursor start() const;

    int virtualLine() const;
    void setVirtualLine(int virtualLine);

    bool isDirty(int viewLine) const;
    bool setDirty(int viewLine, bool dirty = true);

    int width() const;
    int widthOfLastLine() const;

    int viewLineCount() const;
    KateTextLayout viewLine(int viewLine) const;
    int viewLineForColumn(int column) const;

    bool startsInvisibleBlock() const;

    // This variable is used as follows:
    // non-dynamic-wrapping mode: unused
    // dynamic wrapping mode:
    //   first viewLine of a line: the X position of the first non-whitespace char
    //   subsequent viewLines: the X offset from the left of the display.
    //
    // this is used to provide a dynamic-wrapping-retains-indent feature.
    int shiftX() const;
    void setShiftX(int shiftX);

    QTextLayout *layout() const;
    void setLayout(QTextLayout *layout);
    void invalidateLayout();

    bool isLayoutDirty() const;
    void setLayoutDirty(bool dirty = true);

    bool usePlainTextLine() const;
    void setUsePlainTextLine(bool plain = true);

private:
    // Disable copy
    KateLineLayout(const KateLineLayout &copy);

    QTextLayout *takeLayout() const;

    KateRenderer &m_renderer;
    mutable Kate::TextLine m_textLine;
    int m_line;
    int m_virtualLine;
    int m_shiftX;

    QTextLayout *m_layout;
    QList<bool> m_dirtyList;

    bool m_layoutDirty;
    bool m_usePlainTextLine;
};

typedef QExplicitlySharedDataPointer<KateLineLayout> KateLineLayoutPtr;

#endif

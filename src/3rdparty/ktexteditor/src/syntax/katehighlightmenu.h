/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

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

#ifndef KATE_HIGHLIGHTMENU_H
#define KATE_HIGHLIGHTMENU_H

#include <QHash>
#include <QPointer>
#include <QStringList>

#include <KActionMenu>

namespace KTextEditor
{
class DocumentPrivate;
}

class KateHighlightingMenu : public KActionMenu
{
    Q_OBJECT

public:
    KateHighlightingMenu(const QString &text, QObject *parent)
        : KActionMenu(text, parent)
    {
        init();
        setDelayed(false);
    }

    ~KateHighlightingMenu();

    void updateMenu(KTextEditor::DocumentPrivate *doc);

private:
    void init();

    QPointer<KTextEditor::DocumentPrivate> m_doc;
    QStringList subMenusName;
    QStringList names;
    QList<QMenu *> subMenus;
    QList<QAction *> subActions;
    QActionGroup *m_actionGroup;

public Q_SLOTS:
    void slotAboutToShow();

private Q_SLOTS:
    void setHl();
};

#endif

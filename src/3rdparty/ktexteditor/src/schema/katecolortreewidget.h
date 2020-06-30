/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2012-2018 Dominik Haumann <dhaumann@kde.org>

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

#ifndef KATE_COLOR_TREE_WIDGET_H
#define KATE_COLOR_TREE_WIDGET_H

#include <QTreeWidget>

class KateColorItem
{
public:
    KateColorItem()
    {
    }

    QString name;           // translated name
    QString category;       // translated category for tree view hierarchy
    QString whatsThis;      // what's this info
    QString key;            // untranslated id, used as key to save/load from KConfig
    QColor color;           // user visible color
    QColor defaultColor;    // used when "Default" is clicked
    bool useDefault = true; // flag whether to use the default color
};

class KateColorTreeWidget : public QTreeWidget
{
    Q_OBJECT
    friend class KateColorTreeItem;
    friend class KateColorTreeDelegate;

public:
    explicit KateColorTreeWidget(QWidget *parent = nullptr);

public:
    void addColorItem(const KateColorItem &colorItem);
    void addColorItems(const QVector<KateColorItem> &colorItems);

    QVector<KateColorItem> colorItems() const;

    QColor findColor(const QString &key) const;

public Q_SLOTS:
    void selectDefaults();

Q_SIGNALS:
    void changed();

protected:
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) override;
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const override;
};

#endif

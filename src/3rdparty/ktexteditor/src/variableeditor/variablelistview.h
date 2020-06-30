/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2011-2018 Dominik Haumann <dhaumann@kde.org>

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

#ifndef VARIABLE_LIST_VIEW_H
#define VARIABLE_LIST_VIEW_H

#include <QMap>
#include <QScrollArea>

class VariableItem;
class VariableEditor;

class VariableListView : public QScrollArea
{
    Q_OBJECT

public:
    explicit VariableListView(const QString &variableLine, QWidget *parent = nullptr);
    ~VariableListView() override;

    void addItem(VariableItem *item);

    /// always returns the up-to-date variables line
    QString variableLine();

Q_SIGNALS:
    void aboutToHide();
    void changed(); // unused right now

protected:
    void resizeEvent(QResizeEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void parseVariables(const QString &line);

    QVector<VariableItem *> m_items;
    QVector<VariableEditor *> m_editors;

    QMap<QString, QString> m_variables;
};

#endif

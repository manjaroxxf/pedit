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

#include "variablelistview.h"
#include "variableeditor.h"
#include "variableitem.h"

#include <QRegularExpression>
#include <QResizeEvent>

VariableListView::VariableListView(const QString &variableLine, QWidget *parent)
    : QScrollArea(parent)
{
    setBackgroundRole(QPalette::Base);

    setWidget(new QWidget(this));

    parseVariables(variableLine);
}

VariableListView::~VariableListView()
{
}

void VariableListView::parseVariables(const QString &line)
{
    QString tmp = line.trimmed();
    if (tmp.startsWith(QLatin1String("kate:"))) {
        tmp.remove(0, 5);
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList variables = tmp.split(QLatin1Char(';'), QString::SkipEmptyParts);
#else
    QStringList variables = tmp.split(QLatin1Char(';'), Qt::SkipEmptyParts);
#endif

    const QRegularExpression sep(QStringLiteral("\\s+"));
    for (int i = 0; i < variables.size(); ++i) {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        QStringList pair = variables[i].split(sep, QString::SkipEmptyParts);
#else
        QStringList pair = variables[i].split(sep, Qt::SkipEmptyParts);
#endif
        if (pair.size() < 2) {
            continue;
        }
        if (pair.size() > 2) { // e.g. fonts have spaces in the value. Hence, join all value items again
            QString key = pair[0];
            pair.removeAt(0);
            QString value = pair.join(QLatin1Char(' '));
            pair.clear();
            pair << key << value;
        }

        m_variables[pair[0]] = pair[1];
    }
}

void VariableListView::addItem(VariableItem *item)
{
    // overwrite default value when variable exists in modeline
    if (m_variables.contains(item->variable())) {
        item->setValueByString(m_variables[item->variable()]);
        item->setActive(true);
    }

    VariableEditor *editor = item->createEditor(widget());
    editor->setBackgroundRole((m_editors.size() % 2) ? QPalette::AlternateBase : QPalette::Base);

    m_editors << editor;
    m_items << item;

    connect(editor, SIGNAL(valueChanged()), this, SIGNAL(changed()));
}

void VariableListView::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);

    // calculate sum of all editor heights
    int listHeight = 0;
    for (QWidget *w : qAsConst(m_editors)) {
        listHeight += w->sizeHint().height();
    }

    // resize scroll area widget
    QWidget *top = widget();
    top->resize(event->size().width(), listHeight);

    // set client geometries correctly
    int h = 0;
    for (QWidget *w : qAsConst(m_editors)) {
        w->setGeometry(0, h, top->width(), w->sizeHint().height());
        h += w->sizeHint().height();
    }
}

void VariableListView::hideEvent(QHideEvent *event)
{
    if (!event->spontaneous()) {
        emit aboutToHide();
    }
    QScrollArea::hideEvent(event);
}

QString VariableListView::variableLine()
{
    for (int i = 0; i < m_items.size(); ++i) {
        VariableItem *item = m_items[i];

        if (item->isActive()) {
            m_variables[item->variable()] = item->valueAsString();
        } else if (m_variables.contains(item->variable())) {
            m_variables.remove(item->variable());
        }
    }

    QString line;
    QMap<QString, QString>::const_iterator it = m_variables.constBegin();
    while (it != m_variables.constEnd()) {
        if (!line.isEmpty()) {
            line += QLatin1Char(' ');
        }
        line += it.key() + QLatin1Char(' ') + it.value() + QLatin1Char(';');

        ++it;
    }

    line.prepend(QLatin1String("kate: "));

    return line;
}

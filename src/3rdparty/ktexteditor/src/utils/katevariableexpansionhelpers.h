/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright 2019 Dominik Haumann <dhaumann@kde.org>

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

#ifndef KTEXTEDITOR_VARIABLE_EXPANSION_HELPERS_H
#define KTEXTEDITOR_VARIABLE_EXPANSION_HELPERS_H

#include <QDialog>
#include <QPointer>
#include <QString>
#include <QVector>

#include "variable.h"

class QListView;
class QLineEdit;
class QSortFilterProxyModel;
class VariableItemModel;
class TextEditButton;

namespace KTextEditor
{
class View;
class Variable;
}

/**
 * Helper for macro expansion.
 */
namespace KateMacroExpander
{
/**
 * Expands the @p input text based on the @p view.
 * @return the expanded text.
 */
QString expandMacro(const QString &input, KTextEditor::View *view);
}

/**
 * Helper dialog that shows a non-modal dialog listing all available
 * variables. If the user selects a variable, the variable is inserted
 * into the respective widget.
 */
class KateVariableExpansionDialog : public QDialog
{
public:
    KateVariableExpansionDialog(QWidget *parent);
    ~KateVariableExpansionDialog();

    /**
     * Adds @p variable to the expansion list view.
     */
    void addVariable(const KTextEditor::Variable &variable);

    /**
     * Returns true if no variables were added at all to the dialog.
     */
    int isEmpty() const;

    /**
     * Adds @p widget to the list of widgets that trigger showing this dialog.
     */
    void addWidget(QWidget *widget);

protected:
    /**
     * Reimplemented for the following reasons:
     * - Show this dialog if one of the widgets added with addWidget() has focus.
     * - Catch the resize-event for widgets (e.g. QTextEdit) where we manually
     *   added a clickable action in the corner
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

    /**
     * Called whenever a widget was deleted. If all widgets are deleted,
     * this dialog deletes itself via deleteLater().
     */
    void onObjectDeleted(QObject *object);

private:
    QAction *m_showAction;
    QHash<QWidget *, QPointer<TextEditButton>> m_textEditButtons;
    QVector<QObject *> m_widgets;
    QVector<KTextEditor::Variable> m_variables;
    VariableItemModel *m_variableModel;
    QSortFilterProxyModel *m_filterModel;
    QListView *m_listView;
    QLineEdit *m_filterEdit;
};

#endif // KTEXTEDITOR_VARIABLE_EXPANSION_HELPERS_H

// kate: space-indent on; indent-width 4; replace-tabs on;

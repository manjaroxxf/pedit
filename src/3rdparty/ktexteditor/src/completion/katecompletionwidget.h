/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>
    Copyright (C) 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>

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

#ifndef KATECOMPLETIONWIDGET_H
#define KATECOMPLETIONWIDGET_H

#include <QFrame>
#include <QObject>

#include <ktexteditor_export.h>

#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/movingrange.h>

class QToolButton;
class QPushButton;
class QLabel;
class QTimer;

namespace KTextEditor
{
class ViewPrivate;
}
class KateCompletionModel;
class KateCompletionTree;
class KateArgumentHintTree;
class KateArgumentHintModel;

/**
 * This is the code completion's main widget, and also contains the
 * core interface logic.
 *
 * @author Hamish Rodda <rodda@kde.org>
 */
class KTEXTEDITOR_EXPORT KateCompletionWidget : public QFrame
{
    Q_OBJECT

public:
    explicit KateCompletionWidget(KTextEditor::ViewPrivate *parent);
    ~KateCompletionWidget() override;

    KTextEditor::ViewPrivate *view() const;
    KateCompletionTree *treeView() const;

    bool isCompletionActive() const;
    void startCompletion(KTextEditor::CodeCompletionModel::InvocationType invocationType, const QList<KTextEditor::CodeCompletionModel *> &models = QList<KTextEditor::CodeCompletionModel *>());
    void startCompletion(const KTextEditor::Range &word, KTextEditor::CodeCompletionModel *model, KTextEditor::CodeCompletionModel::InvocationType invocationType = KTextEditor::CodeCompletionModel::ManualInvocation);
    void startCompletion(const KTextEditor::Range &word,
                         const QList<KTextEditor::CodeCompletionModel *> &models = QList<KTextEditor::CodeCompletionModel *>(),
                         KTextEditor::CodeCompletionModel::InvocationType invocationType = KTextEditor::CodeCompletionModel::ManualInvocation);
    void userInvokedCompletion();

public Q_SLOTS:
    // Executed when return is pressed while completion is active.
    void execute();
    void cursorDown();
    void cursorUp();

public:
    void tab(bool shift);

    /// Returns whether the current item was expanded/unexpanded
    bool toggleExpanded(bool forceExpand = false, bool forceUnExpand = false);

    const KateCompletionModel *model() const;
    KateCompletionModel *model();

    void registerCompletionModel(KTextEditor::CodeCompletionModel *model);
    void unregisterCompletionModel(KTextEditor::CodeCompletionModel *model);
    bool isCompletionModelRegistered(KTextEditor::CodeCompletionModel *model) const;

    int automaticInvocationDelay() const;
    void setAutomaticInvocationDelay(int delay);

    struct CompletionRange {
        CompletionRange()
        {
        }
        explicit CompletionRange(KTextEditor::MovingRange *r)
            : range(r)
        {
        }

        bool operator==(const CompletionRange &rhs) const
        {
            return range->toRange() == rhs.range->toRange();
        }

        KTextEditor::MovingRange *range = nullptr;
        // Whenever the cursor goes before this position, the completion is stopped, unless it is invalid.
        KTextEditor::Cursor leftBoundary;
    };

    KTextEditor::MovingRange *completionRange(KTextEditor::CodeCompletionModel *model = nullptr) const;
    QMap<KTextEditor::CodeCompletionModel *, CompletionRange> completionRanges() const;

    // Navigation
    void pageDown();
    void pageUp();
    void top();
    void bottom();

    QWidget *currentEmbeddedWidget();

    bool canExpandCurrentItem() const;

    bool canCollapseCurrentItem() const;

    void setCurrentItemExpanded(bool);

    // Returns true if a screen border has been hit
    bool updatePosition(bool force = false);

    bool eventFilter(QObject *watched, QEvent *event) override;

    KateArgumentHintTree *argumentHintTree() const;

    KateArgumentHintModel *argumentHintModel() const;

    /// Called by KateViewInternal, because we need the specific information from the event.

    void updateHeight();

public Q_SLOTS:
    void waitForModelReset();

    void abortCompletion();
    void showConfig();
    /*    void viewFocusIn();
        void viewFocusOut();*/
    void updatePositionSlot();
    void automaticInvocation();

    /*    void updateFocus();*/
    void argumentHintsChanged(bool hasContent);

    bool navigateUp();
    bool navigateDown();
    bool navigateLeft();
    bool navigateRight();
    bool navigateAccept();
    bool navigateBack();

    bool hadNavigation() const;
    void resetHadNavigation();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private Q_SLOTS:
    void completionModelReset();
    void modelDestroyed(QObject *model);
    void modelContentChanged();
    void cursorPositionChanged();
    void modelReset();
    void rowsInserted(const QModelIndex &parent, int row, int rowEnd);
    void viewFocusOut();

    void wrapLine(const KTextEditor::Cursor &position);
    void unwrapLine(int line);
    void insertText(const KTextEditor::Cursor &position, const QString &text);
    void removeText(const KTextEditor::Range &range);

private:
    void updateAndShow();
    void updateArgumentHintGeometry();
    QModelIndex selectedIndex() const;

    void clear();
    // Switch cursor between argument-hint list / completion-list
    void switchList();
    KTextEditor::Range determineRange() const;
    void completionRangeChanged(KTextEditor::CodeCompletionModel *, const KTextEditor::Range &word);

    void deleteCompletionRanges();

    QList<KTextEditor::CodeCompletionModel *> m_sourceModels;
    KateCompletionModel *m_presentationModel;

    QMap<KTextEditor::CodeCompletionModel *, CompletionRange> m_completionRanges;
    QSet<KTextEditor::CodeCompletionModel *> m_waitingForReset;

    KTextEditor::Cursor m_lastCursorPosition;

    KateCompletionTree *m_entryList;
    KateArgumentHintModel *m_argumentHintModel;
    KateArgumentHintTree *m_argumentHintTree;

    QTimer *m_automaticInvocationTimer;
    // QTimer* m_updateFocusTimer;
    QWidget *m_statusBar;
    QToolButton *m_sortButton;
    QLabel *m_sortText;
    QToolButton *m_filterButton;
    QLabel *m_filterText;
    QPushButton *m_configButton;

    KTextEditor::Cursor m_automaticInvocationAt;
    QString m_automaticInvocationLine;
    int m_automaticInvocationDelay;
    bool m_filterInstalled;

    class KateCompletionConfig *m_configWidget;
    bool m_lastInsertionByUser;
    bool m_inCompletionList; // Are we in the completion-list? If not, we're in the argument-hint list
    bool m_isSuspended;
    bool m_dontShowArgumentHints; // Used temporarily to prevent flashing
    bool m_needShow;

    bool m_hadCompletionNavigation;

    bool m_haveExactMatch;

    bool m_noAutoHide;

    /**
     * is a completion edit ongoing?
     */
    bool m_completionEditRunning;

    int m_expandedAddedHeightBase;
    KTextEditor::CodeCompletionModel::InvocationType m_lastInvocationType;
};

#endif

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

#ifndef KATECOMPLETIONMODEL_H
#define KATECOMPLETIONMODEL_H

#include <QAbstractProxyModel>
#include <QList>
#include <QPair>

#include <ktexteditor/codecompletionmodel.h>

#include "expandingtree/expandingwidgetmodel.h"
#include <ktexteditor_export.h>

class KateCompletionWidget;
class KateArgumentHintModel;
namespace KTextEditor
{
class ViewPrivate;
}
class QWidget;
class QTextEdit;
class QTimer;
class HierarchicalModelHandler;

/**
 * This class has the responsibility for filtering, sorting, and manipulating
 * code completion data provided by a CodeCompletionModel.
 *
 * @author Hamish Rodda <rodda@kde.org>
 */
class KTEXTEDITOR_EXPORT KateCompletionModel : public ExpandingWidgetModel
{
    Q_OBJECT

public:
    explicit KateCompletionModel(KateCompletionWidget *parent = nullptr);
    ~KateCompletionModel() override;

    QList<KTextEditor::CodeCompletionModel *> completionModels() const;
    void clearCompletionModels();
    void addCompletionModel(KTextEditor::CodeCompletionModel *model);
    void setCompletionModel(KTextEditor::CodeCompletionModel *model);
    void setCompletionModels(const QList<KTextEditor::CodeCompletionModel *> &models);
    void removeCompletionModel(KTextEditor::CodeCompletionModel *model);

    KTextEditor::ViewPrivate *view() const;
    KateCompletionWidget *widget() const;

    QString currentCompletion(KTextEditor::CodeCompletionModel *model) const;
    void setCurrentCompletion(KTextEditor::CodeCompletionModel *model, const QString &completion);

    Qt::CaseSensitivity matchCaseSensitivity() const;
    Qt::CaseSensitivity exactMatchCaseSensitivity() const;
    void setMatchCaseSensitivity(Qt::CaseSensitivity match_cs);
    void setMatchCaseSensitivity(Qt::CaseSensitivity match_cs, Qt::CaseSensitivity exact_match_cs);

    static QString columnName(int column);
    int translateColumn(int sourceColumn) const;

    static QString propertyName(KTextEditor::CodeCompletionModel::CompletionProperty property);

    /// Returns a common prefix for all current visible completion entries
    /// If there is no common prefix, extracts the next useful prefix for the selected index
    QString commonPrefix(QModelIndex selectedIndex) const;

    void rowSelected(const QModelIndex &row) override;

    bool indexIsItem(const QModelIndex &index) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    virtual bool hasIndex(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    // Disabled in case of bugs, reenable once fully debugged.
    // virtual QMap<int, QVariant> itemData ( const QModelIndex & index ) const;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // Disabled in case of bugs, reenable once fully debugged.
    // virtual QModelIndex sibling ( int row, int column, const QModelIndex & index ) const;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    /// Maps from this display-model into the appropriate source code-completion model
    virtual QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

    /// Maps from an index in a source-model to the index of the item in this display-model
    virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

    // Sorting
    bool isSortingEnabled() const;
    bool isSortingAlphabetical() const;
    bool isSortingByInheritanceDepth() const;
    void setSortingByInheritanceDepth(bool byIneritance);
    void setSortingAlphabetical(bool alphabetical);

    Qt::CaseSensitivity sortingCaseSensitivity() const;
    void setSortingCaseSensitivity(Qt::CaseSensitivity cs);

    bool isSortingReverse() const;
    void setSortingReverse(bool reverse);

    // Filtering
    bool isFilteringEnabled() const;

    bool filterContextMatchesOnly() const;
    void setFilterContextMatchesOnly(bool filter);

    bool filterByAttribute() const;
    void setFilterByAttribute(bool filter);

    KTextEditor::CodeCompletionModel::CompletionProperties filterAttributes() const;
    void setFilterAttributes(KTextEditor::CodeCompletionModel::CompletionProperties attributes);

    // A maximum depth of <= 0 equals don't filter by inheritance depth (i.e. infinity) and is default
    int maximumInheritanceDepth() const;
    void setMaximumInheritanceDepth(int maxDepth);

    // Grouping
    bool isGroupingEnabled() const;

    enum gm { ScopeType = 0x1, Scope = 0x2, AccessType = 0x4, ItemType = 0x8 };

    enum { // An own property that will be used to mark the best-matches group internally
        BestMatchesProperty = 2 * KTextEditor::CodeCompletionModel::LastProperty
    };

    Q_DECLARE_FLAGS(GroupingMethods, gm)

    static const int ScopeTypeMask = 0x380000;
    static const int AccessTypeMask = 0x7;
    static const int ItemTypeMask = 0xfe0;

    GroupingMethods groupingMethod() const;
    void setGroupingMethod(GroupingMethods m);

    bool accessIncludeConst() const;
    void setAccessIncludeConst(bool include);
    bool accessIncludeStatic() const;
    void setAccessIncludeStatic(bool include);
    bool accessIncludeSignalSlot() const;
    void setAccessIncludeSignalSlot(bool include);

    // Column merging
    bool isColumnMergingEnabled() const;

    const QList<QList<int>> &columnMerges() const;
    void setColumnMerges(const QList<QList<int>> &columnMerges);

    void debugStats();

    /// Returns whether one of the filtered items exactly matches its completion string
    bool shouldMatchHideCompletionList() const;

    uint filteredItemCount() const;

protected:
    int contextMatchQuality(const QModelIndex &index) const override;

Q_SIGNALS:
    void expandIndex(const QModelIndex &index);
    // Emitted whenever something has changed about the group of argument-hints
    void argumentHintsChanged();

public Q_SLOTS:
    void setSortingEnabled(bool enable);
    void setFilteringEnabled(bool enable);
    void setGroupingEnabled(bool enable);
    void setColumnMergingEnabled(bool enable);

private Q_SLOTS:
    void slotRowsInserted(const QModelIndex &parent, int start, int end);
    void slotRowsRemoved(const QModelIndex &parent, int start, int end);
    void slotModelReset();

    // Updates the best-matches group
    void updateBestMatches();
    // Makes sure that the ungrouped group contains each item only once
    // Must only be called right after the group was created
    void makeGroupItemsUnique(bool onlyFiltered = false);

private:
    typedef QPair<KTextEditor::CodeCompletionModel *, QModelIndex> ModelRow;
    virtual int contextMatchQuality(const ModelRow &sourceRow) const;

    QTreeView *treeView() const override;

    friend class KateArgumentHintModel;
    ModelRow modelRowPair(const QModelIndex &index) const;

    // Represents a source row; provides sorting method
    class Item
    {
    public:
        Item(bool doInitialMatch, KateCompletionModel *model, const HierarchicalModelHandler &handler, ModelRow sourceRow);

        bool isValid() const;
        // Returns true if the item is not filtered and matches the current completion string
        bool isVisible() const;
        // Returns whether the item is filtered or not
        bool isFiltered() const;
        // Returns whether the item matches the current completion string
        bool isMatching() const;

        bool filter();

        enum MatchType { NoMatch = 0, PerfectMatch, StartsWithMatch, AbbreviationMatch, ContainsMatch };
        MatchType match();

        const ModelRow &sourceRow() const;

        // Sorting operator
        bool operator<(const Item &rhs) const;

        bool haveExactMatch() const
        {
            return m_haveExactMatch;
        }

        void clearExactMatch()
        {
            m_haveExactMatch = false;
        }

        QString name() const
        {
            return m_nameColumn;
        }

    private:
        KateCompletionModel *model;
        ModelRow m_sourceRow;

        mutable QString m_nameColumn;

        int inheritanceDepth;

        // True when currently matching completion string
        MatchType matchCompletion;
        // True when passes all active filters
        bool matchFilters;
        bool m_haveExactMatch;
        bool m_unimportant;

        QString completionSortingName() const;
    };

public:
    // Grouping and sorting of rows
    class Group
    {
    public:
        explicit Group(const QString &title, int attribute, KateCompletionModel *model);

        void addItem(const Item &i, bool notifyModel = false);
        /// Removes the item specified by \a row.  Returns true if a change was made to rows.
        bool removeItem(const ModelRow &row);
        void resort();
        void refilter();
        void clear();
        // Returns whether this group should be ordered before other
        bool orderBefore(Group *other) const;
        // Returns a number that can be used for ordering
        int orderNumber() const;

        /// Returns the row in the this group's filtered list of the given model-row in a source-model
        ///-1 if the item is not in the filtered list
        ///@todo Implement an efficient way of doing this map, that does _not_ iterate over all items!
        int rowOf(ModelRow item)
        {
            for (int a = 0; a < filtered.size(); ++a)
                if (filtered[a].sourceRow() == item) {
                    return a;
                }
            return -1;
        }

        KateCompletionModel *model;
        int attribute;
        QString title, scope;
        QList<Item> filtered;
        QList<Item> prefilter;
        bool isEmpty;
        //-1 if none was set
        int customSortingKey;
    };

    bool hasGroups() const;

private:
    QString commonPrefixInternal(const QString &forcePrefix) const;
    /// @note performs model reset
    void createGroups();
    /// Creates all sub-items of index i, or the item corresponding to index i. Returns the affected groups.
    /// i must be an index in the source model
    QSet<Group *> createItems(const HierarchicalModelHandler &, const QModelIndex &i, bool notifyModel = false);
    /// Deletes all sub-items of index i, or the item corresponding to index i. Returns the affected groups.
    /// i must be an index in the source model
    QSet<Group *> deleteItems(const QModelIndex &i);
    Group *createItem(const HierarchicalModelHandler &, const QModelIndex &i, bool notifyModel = false);
    /// @note Make sure you're in a {begin,end}ResetModel block when calling this!
    void clearGroups();
    void hideOrShowGroup(Group *g, bool notifyModel = false);
    /// When forceGrouping is enabled, all given attributes will be used for grouping, regardless of the completion settings.
    Group *fetchGroup(int attribute, const QString &scope = QString(), bool forceGrouping = false);
    // If this returns nonzero on an index, the index is the header of the returned group
    Group *groupForIndex(const QModelIndex &index) const;
    inline Group *groupOfParent(const QModelIndex &child) const
    {
        return static_cast<Group *>(child.internalPointer());
    }
    QModelIndex indexForRow(Group *g, int row) const;
    QModelIndex indexForGroup(Group *g) const;

    enum changeTypes { Broaden, Narrow, Change };

    // Returns whether the model needs to be reset
    void changeCompletions(Group *g, changeTypes changeType, bool notifyModel);

    bool hasCompletionModel() const;

    /// Removes attributes not used in grouping from the input \a attribute
    int groupingAttributes(int attribute) const;
    int countBits(int value) const;

    void resort();
    void refilter();

    static bool matchesAbbreviation(const QString &word, const QString &typed, Qt::CaseSensitivity caseSensitive);

    bool m_hasGroups = false;

    // ### Runtime state
    // General
    QList<KTextEditor::CodeCompletionModel *> m_completionModels;
    QMap<KTextEditor::CodeCompletionModel *, QString> m_currentMatch;

    // Column merging
    QList<QList<int>> m_columnMerges;

    QTimer *m_updateBestMatchesTimer;

    Group *m_ungrouped;
    Group *m_argumentHints; // The argument-hints will be passed on to another model, to be shown in another widget
    Group *m_bestMatches;   // A temporary group used for holding the best matches of all visible items

    // Storing the sorted order
    QList<Group *> m_rowTable;
    QList<Group *> m_emptyGroups;
    // Quick access to each specific group (if it exists)
    QMultiHash<int, Group *> m_groupHash;
    // Maps custom group-names to their specific groups
    QHash<QString, Group *> m_customGroupHash;

    // ### Configurable state
    // Matching
    Qt::CaseSensitivity m_matchCaseSensitivity = Qt::CaseInsensitive;
    Qt::CaseSensitivity m_exactMatchCaseSensitivity = Qt::CaseInsensitive; // Must be equal to or stricter than m_matchCaseSensitivity.

    // Sorting
    bool m_sortingEnabled = false;
    bool m_sortingAlphabetical = false;
    bool m_isSortingByInheritance = false;
    Qt::CaseSensitivity m_sortingCaseSensitivity = Qt::CaseInsensitive;
    QHash<int, QList<int>> m_sortingGroupingOrder;

    // Filtering
    bool m_filteringEnabled = false;
    bool m_filterContextMatchesOnly = false;
    bool m_filterByAttribute = false;
    KTextEditor::CodeCompletionModel::CompletionProperties m_filterAttributes;
    int m_maximumInheritanceDepth = 0;

    // Grouping
    bool m_groupingEnabled = false;
    GroupingMethods m_groupingMethod;
    bool m_accessConst = false, m_accessStatic = false, m_accesSignalSlot = false;

    // Column merging
    bool m_columnMergingEnabled = false /*, m_haveExactMatch*/;

    friend class CompletionTest;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KateCompletionModel::GroupingMethods)

#endif

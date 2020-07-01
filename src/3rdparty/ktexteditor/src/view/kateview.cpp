/* This file is part of the KDE libraries
   Copyright (C) 2009 Michel Ludwig <michel.ludwig@kdemail.net>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001-2010 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// BEGIN includes
#include "kateview.h"

#include "export/exporter.h"
#include "inlinenotedata.h"
#include "kateabstractinputmode.h"
#include "kateautoindent.h"
#include "katebookmarks.h"
#include "katebuffer.h"
#include "katecompletionwidget.h"
#include "kateconfig.h"
#include "katedialogs.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katehighlightmenu.h"
#include "katekeywordcompletion.h"
#include "katelayoutcache.h"
#include "katemessagewidget.h"
#include "katemodemenu.h"
#include "katepartdebug.h"
#include "katerenderer.h"
#include "kateschema.h"
#include "katestatusbar.h"
#include "katetemplatehandler.h"
#include "katetextline.h"
#include "kateundomanager.h"
#include "kateviewhelpers.h"
#include "kateviewinternal.h"
#include "katewordcompletion.h"
#include "printing/kateprinter.h"
#include "script/katescriptaction.h"
#include "script/katescriptmanager.h"
#include "spellcheck/spellcheck.h"
#include "spellcheck/spellcheckdialog.h"
#include "spellcheck/spellingmenu.h"

#include <KTextEditor/Message>
#include <ktexteditor/inlinenoteprovider.h>

#include <KParts/Event>

#include <KActionCollection>
#include <KCharsets>
#include <KConfig>
#include <KConfigGroup>
#include <KCursor>
#include <KMessageBox>
#include <KSelectAction>
#include <KStandardAction>
#include <KStandardShortcut>
#include <KToggleAction>
#include <KXMLGUIFactory>

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QKeyEvent>
#include <QLayout>
#include <QMimeData>
#include <QPainter>
#include <QRegularExpression>
#include <QToolTip>

//#define VIEW_RANGE_DEBUG

// END includes

namespace
{
bool hasCommentInFirstLine(KTextEditor::DocumentPrivate *doc)
{
    const Kate::TextLine &line = doc->kateTextLine(0);
    Q_ASSERT(line);
    return doc->isComment(0, line->firstChar());
}

}

void KTextEditor::ViewPrivate::blockFix(KTextEditor::Range &range)
{
    if (range.start().column() > range.end().column()) {
        int tmp = range.start().column();
        range.setStart(KTextEditor::Cursor(range.start().line(), range.end().column()));
        range.setEnd(KTextEditor::Cursor(range.end().line(), tmp));
    }
}

KTextEditor::ViewPrivate::ViewPrivate(KTextEditor::DocumentPrivate *doc, QWidget *parent, KTextEditor::MainWindow *mainWindow)
    : KTextEditor::View(this, parent)
    , m_completionWidget(nullptr)
    , m_annotationModel(nullptr)
    , m_hasWrap(false)
    , m_doc(doc)
    , m_textFolding(doc->buffer())
    , m_config(new KateViewConfig(this))
    , m_renderer(new KateRenderer(doc, m_textFolding, this))
    , m_viewInternal(new KateViewInternal(this))
    , m_spell(new KateSpellCheckDialog(this))
    , m_bookmarks(new KateBookmarks(this))
    , m_topSpacer(new QSpacerItem(0, 0))
    , m_leftSpacer(new QSpacerItem(0, 0))
    , m_rightSpacer(new QSpacerItem(0, 0))
    , m_bottomSpacer(new QSpacerItem(0, 0))
    , m_startingUp(true)
    , m_updatingDocumentConfig(false)
    , m_selection(m_doc->buffer(), KTextEditor::Range::invalid(), Kate::TextRange::ExpandLeft, Kate::TextRange::AllowEmpty)
    , blockSelect(false)
    , m_bottomViewBar(nullptr)
    , m_gotoBar(nullptr)
    , m_dictionaryBar(nullptr)
    , m_spellingMenu(new KateSpellingMenu(this))
    , m_userContextMenuSet(false)
    , m_delayedUpdateTriggered(false)
    , m_lineToUpdateMin(-1)
    , m_lineToUpdateMax(-1)
    , m_mainWindow(mainWindow ? mainWindow : KTextEditor::EditorPrivate::self()->dummyMainWindow()) // use dummy window if no window there!
    , m_statusBar(nullptr)
    , m_temporaryAutomaticInvocationDisabled(false)
    , m_autoFoldedFirstLine(false)
{
    // queued connect to collapse view updates for range changes, INIT THIS EARLY ENOUGH!
    connect(this, SIGNAL(delayedUpdateOfView()), this, SLOT(slotDelayedUpdateOfView()), Qt::QueuedConnection);

    KXMLGUIClient::setComponentName(KTextEditor::EditorPrivate::self()->aboutData().componentName(), KTextEditor::EditorPrivate::self()->aboutData().displayName());

    // selection if for this view only and will invalidate if becoming empty
    m_selection.setView(this);

    // use z depth defined in moving ranges interface
    m_selection.setZDepth(-100000.0);

    KTextEditor::EditorPrivate::self()->registerView(this);

    /**
     * try to let the main window, if any, create a view bar for this view
     */
    QWidget *bottomBarParent = m_mainWindow->createViewBar(this);

    m_bottomViewBar = new KateViewBar(bottomBarParent != nullptr, bottomBarParent ? bottomBarParent : this, this);

    // ugly workaround:
    // Force the layout to be left-to-right even on RTL desktop, as discussed
    // on the mailing list. This will cause the lines and icons panel to be on
    // the left, even for Arabic/Hebrew/Farsi/whatever users.
    setLayoutDirection(Qt::LeftToRight);

    m_bottomViewBar->installEventFilter(m_viewInternal);

    // add KateMessageWidget for KTE::MessageInterface immediately above view
    m_messageWidgets[KTextEditor::Message::AboveView] = new KateMessageWidget(this);
    m_messageWidgets[KTextEditor::Message::AboveView]->hide();

    // add KateMessageWidget for KTE::MessageInterface immediately above view
    m_messageWidgets[KTextEditor::Message::BelowView] = new KateMessageWidget(this);
    m_messageWidgets[KTextEditor::Message::BelowView]->hide();

    // add bottom viewbar...
    if (bottomBarParent) {
        m_mainWindow->addWidgetToViewBar(this, m_bottomViewBar);
    }

    // add layout for floating message widgets to KateViewInternal
    m_notificationLayout = new KateMessageLayout(m_viewInternal);
    m_notificationLayout->setContentsMargins(20, 20, 20, 20);
    m_viewInternal->setLayout(m_notificationLayout);

    // this really is needed :)
    m_viewInternal->updateView();

    doc->addView(this);

    setFocusProxy(m_viewInternal);
    setFocusPolicy(Qt::StrongFocus);

    setXMLFile(QStringLiteral("katepart5ui.rc"));

    setupConnections();
    setupActions();

    // auto word completion
    new KateWordCompletionView(this, actionCollection());

    // update the enabled state of the undo/redo actions...
    slotUpdateUndo();

    /**
     * create the status bar of this view
     * do this after action creation, we use some of them!
     */
    toggleStatusBar();

    m_startingUp = false;
    updateConfig();

    slotHlChanged();
    KCursor::setAutoHideCursor(m_viewInternal, true);

    for (auto messageWidget : m_messageWidgets) {
        if (messageWidget) {
            // user interaction (scrolling) starts notification auto-hide timer
            connect(this, SIGNAL(displayRangeChanged(KTextEditor::ViewPrivate *)), messageWidget, SLOT(startAutoHideTimer()));

            // user interaction (cursor navigation) starts notification auto-hide timer
            connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View *, KTextEditor::Cursor)), messageWidget, SLOT(startAutoHideTimer()));
        }
    }

    // folding restoration on reload
    connect(m_doc, SIGNAL(aboutToReload(KTextEditor::Document *)), SLOT(saveFoldingState()));
    connect(m_doc, SIGNAL(reloaded(KTextEditor::Document *)), SLOT(applyFoldingState()));

    connect(m_doc, &KTextEditor::DocumentPrivate::reloaded, this, &KTextEditor::ViewPrivate::slotDocumentReloaded);
    connect(m_doc, &KTextEditor::DocumentPrivate::aboutToReload, this, &KTextEditor::ViewPrivate::slotDocumentAboutToReload);

    // update highlights on scrolling and co
    connect(this, SIGNAL(displayRangeChanged(KTextEditor::ViewPrivate *)), this, SLOT(createHighlights()));

    // clear highlights on reload
    connect(m_doc, SIGNAL(aboutToReload(KTextEditor::Document *)), SLOT(clearHighlights()));

    // setup layout
    setupLayout();
}

KTextEditor::ViewPrivate::~ViewPrivate()
{
    // invalidate update signal
    m_delayedUpdateTriggered = false;

    /**
     * de-register views early from global collections
     * otherwise we might "use" them again during destruction in a half-valid state
     * see e.g. bug 422546
     * Kate::TextBuffer::notifyAboutRangeChange will access views() in a chain during
     * deletion of m_viewInternal
     */
    doc()->removeView(this);
    KTextEditor::EditorPrivate::self()->deregisterView(this);

    // remove from xmlgui factory, to be safe
    if (factory()) {
        factory()->removeClient(this);
    }

    // delete internal view before view bar!
    delete m_viewInternal;

    /**
     * remove view bar again, if needed
     */
    m_mainWindow->deleteViewBar(this);
    m_bottomViewBar = nullptr;

    delete m_renderer;

    delete m_config;
}

void KTextEditor::ViewPrivate::toggleStatusBar()
{
    /**
     * if there, delete it
     */
    if (m_statusBar) {
        bottomViewBar()->removePermanentBarWidget(m_statusBar);
        delete m_statusBar;
        m_statusBar = nullptr;
        emit statusBarEnabledChanged(this, false);
        return;
    }

    /**
     * else: create it
     */
    m_statusBar = new KateStatusBar(this);
    bottomViewBar()->addPermanentBarWidget(m_statusBar);
    emit statusBarEnabledChanged(this, true);
}

void KTextEditor::ViewPrivate::setupLayout()
{
    // delete old layout if any
    if (layout()) {
        delete layout();

        /**
         *  need to recreate spacers because they are deleted with
         *  their belonging layout
         */
        m_topSpacer = new QSpacerItem(0, 0);
        m_leftSpacer = new QSpacerItem(0, 0);
        m_rightSpacer = new QSpacerItem(0, 0);
        m_bottomSpacer = new QSpacerItem(0, 0);
    }

    // set margins
    QStyleOptionFrame opt;
    opt.initFrom(this);
    opt.frameShape = QFrame::StyledPanel;
    opt.state |= QStyle::State_Sunken;
    const int margin = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this);
    m_topSpacer->changeSize(0, margin, QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_leftSpacer->changeSize(margin, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    m_rightSpacer->changeSize(margin, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    m_bottomSpacer->changeSize(0, margin, QSizePolicy::Minimum, QSizePolicy::Fixed);

    // define layout
    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    const bool frameAroundContents = style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, &opt, this);
    if (frameAroundContents) {
        // top message widget
        layout->addWidget(m_messageWidgets[KTextEditor::Message::AboveView], 0, 0, 1, 5);

        // top spacer
        layout->addItem(m_topSpacer, 1, 0, 1, 4);

        // left spacer
        layout->addItem(m_leftSpacer, 2, 0, 1, 1);

        // left border
        layout->addWidget(m_viewInternal->m_leftBorder, 2, 1, 1, 1);

        // view
        layout->addWidget(m_viewInternal, 2, 2, 1, 1);

        // right spacer
        layout->addItem(m_rightSpacer, 2, 3, 1, 1);

        // bottom spacer
        layout->addItem(m_bottomSpacer, 3, 0, 1, 4);

        // vertical scrollbar
        layout->addWidget(m_viewInternal->m_lineScroll, 1, 4, 3, 1);

        // horizontal scrollbar
        layout->addWidget(m_viewInternal->m_columnScroll, 4, 0, 1, 4);

        // dummy
        layout->addWidget(m_viewInternal->m_dummy, 4, 4, 1, 1);

        // bottom message
        layout->addWidget(m_messageWidgets[KTextEditor::Message::BelowView], 5, 0, 1, 5);

        // bottom viewbar
        if (m_bottomViewBar->parentWidget() == this) {
            layout->addWidget(m_bottomViewBar, 6, 0, 1, 5);
        }

        // stretch
        layout->setColumnStretch(2, 1);
        layout->setRowStretch(2, 1);

        // adjust scrollbar background
        m_viewInternal->m_lineScroll->setBackgroundRole(QPalette::Window);
        m_viewInternal->m_lineScroll->setAutoFillBackground(false);

        m_viewInternal->m_columnScroll->setBackgroundRole(QPalette::Window);
        m_viewInternal->m_columnScroll->setAutoFillBackground(false);

    } else {
        // top message widget
        layout->addWidget(m_messageWidgets[KTextEditor::Message::AboveView], 0, 0, 1, 5);

        // top spacer
        layout->addItem(m_topSpacer, 1, 0, 1, 5);

        // left spacer
        layout->addItem(m_leftSpacer, 2, 0, 1, 1);

        // left border
        layout->addWidget(m_viewInternal->m_leftBorder, 2, 1, 1, 1);

        // view
        layout->addWidget(m_viewInternal, 2, 2, 1, 1);

        // vertical scrollbar
        layout->addWidget(m_viewInternal->m_lineScroll, 2, 3, 1, 1);

        // right spacer
        layout->addItem(m_rightSpacer, 2, 4, 1, 1);

        // horizontal scrollbar
        layout->addWidget(m_viewInternal->m_columnScroll, 3, 1, 1, 2);

        // dummy
        layout->addWidget(m_viewInternal->m_dummy, 3, 3, 1, 1);

        // bottom spacer
        layout->addItem(m_bottomSpacer, 4, 0, 1, 5);

        // bottom message
        layout->addWidget(m_messageWidgets[KTextEditor::Message::BelowView], 5, 0, 1, 5);

        // bottom viewbar
        if (m_bottomViewBar->parentWidget() == this) {
            layout->addWidget(m_bottomViewBar, 6, 0, 1, 5);
        }

        // stretch
        layout->setColumnStretch(2, 1);
        layout->setRowStretch(2, 1);

        // adjust scrollbar background
        m_viewInternal->m_lineScroll->setBackgroundRole(QPalette::Base);
        m_viewInternal->m_lineScroll->setAutoFillBackground(true);

        m_viewInternal->m_columnScroll->setBackgroundRole(QPalette::Base);
        m_viewInternal->m_columnScroll->setAutoFillBackground(true);
    }
}

void KTextEditor::ViewPrivate::setupConnections()
{
    connect(m_doc, SIGNAL(undoChanged()), this, SLOT(slotUpdateUndo()));
    connect(m_doc, SIGNAL(highlightingModeChanged(KTextEditor::Document *)), this, SLOT(slotHlChanged()));
    connect(m_doc, SIGNAL(canceled(QString)), this, SLOT(slotSaveCanceled(QString)));
    connect(m_viewInternal, SIGNAL(dropEventPass(QDropEvent *)), this, SIGNAL(dropEventPass(QDropEvent *)));

    connect(m_doc, SIGNAL(annotationModelChanged(KTextEditor::AnnotationModel *, KTextEditor::AnnotationModel *)), m_viewInternal->m_leftBorder, SLOT(annotationModelChanged(KTextEditor::AnnotationModel *, KTextEditor::AnnotationModel *)));
}

void KTextEditor::ViewPrivate::goToPreviousEditingPosition()
{
    auto c = doc()->lastEditingPosition(KTextEditor::DocumentPrivate::Previous, cursorPosition());
    if (c.isValid()) {
        setCursorPosition(c);
    }
}

void KTextEditor::ViewPrivate::goToNextEditingPosition()
{
    auto c = doc()->lastEditingPosition(KTextEditor::DocumentPrivate::Next, cursorPosition());
    if (c.isValid()) {
        setCursorPosition(c);
    }
}
void KTextEditor::ViewPrivate::setupActions()
{
    KActionCollection *ac = actionCollection();
    QAction *a;

    m_toggleWriteLock = nullptr;

    m_cut = a = ac->addAction(KStandardAction::Cut, this, SLOT(cut()));
    a->setWhatsThis(i18n("Cut the selected text and move it to the clipboard"));

    m_paste = a = ac->addAction(KStandardAction::Paste, this, SLOT(paste()));
    a->setWhatsThis(i18n("Paste previously copied or cut clipboard contents"));

    m_copy = a = ac->addAction(KStandardAction::Copy, this, SLOT(copy()));
    a->setWhatsThis(i18n("Use this command to copy the currently selected text to the system clipboard."));

    m_pasteMenu = ac->addAction(QStringLiteral("edit_paste_menu"), new KatePasteMenu(i18n("Clipboard &History"), this));
    connect(KTextEditor::EditorPrivate::self(), SIGNAL(clipboardHistoryChanged()), this, SLOT(slotClipboardHistoryChanged()));

    if (!doc()->readOnly()) {
        a = ac->addAction(KStandardAction::Save, m_doc, SLOT(documentSave()));
        a->setWhatsThis(i18n("Save the current document"));

        a = m_editUndo = ac->addAction(KStandardAction::Undo, m_doc, SLOT(undo()));
        a->setWhatsThis(i18n("Revert the most recent editing actions"));

        a = m_editRedo = ac->addAction(KStandardAction::Redo, m_doc, SLOT(redo()));
        a->setWhatsThis(i18n("Revert the most recent undo operation"));

        // Tools > Scripts
        // stored inside scoped pointer to ensure we destruct it early enough as it does internal cleanups of other child objects
        m_scriptActionMenu.reset(new KateScriptActionMenu(this, i18n("&Scripts")));
        ac->addAction(QStringLiteral("tools_scripts"), m_scriptActionMenu.data());

        a = ac->addAction(QStringLiteral("tools_apply_wordwrap"));
        a->setText(i18n("Apply &Word Wrap"));
        a->setWhatsThis(
            i18n("Use this to wrap the current line, or to reformat the selected lines as paragraph, "
                 "to fit the 'Wrap words at' setting in the configuration dialog.<br /><br />"
                 "This is a static word wrap, meaning the document is changed."));
        connect(a, SIGNAL(triggered(bool)), SLOT(applyWordWrap()));

        a = ac->addAction(QStringLiteral("tools_cleanIndent"));
        a->setText(i18n("&Clean Indentation"));
        a->setWhatsThis(
            i18n("Use this to clean the indentation of a selected block of text (only tabs/only spaces).<br /><br />"
                 "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
        connect(a, SIGNAL(triggered(bool)), SLOT(cleanIndent()));

        a = ac->addAction(QStringLiteral("tools_align"));
        a->setText(i18n("&Align"));
        a->setWhatsThis(i18n("Use this to align the current line or block of text to its proper indent level."));
        connect(a, SIGNAL(triggered(bool)), SLOT(align()));

        a = ac->addAction(QStringLiteral("tools_comment"));
        a->setText(i18n("C&omment"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_D));
        a->setWhatsThis(
            i18n("This command comments out the current line or a selected block of text.<br /><br />"
                 "The characters for single/multiple line comments are defined within the language's highlighting."));
        connect(a, SIGNAL(triggered(bool)), SLOT(comment()));

        a = ac->addAction(QStringLiteral("Previous Editing Line"));
        a->setText(i18n("Go to previous editing line"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_E));
        connect(a, SIGNAL(triggered(bool)), SLOT(goToPreviousEditingPosition()));

        a = ac->addAction(QStringLiteral("Next Editing Line"));
        a->setText(i18n("Go to next editing line"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_E));
        connect(a, SIGNAL(triggered(bool)), SLOT(goToNextEditingPosition()));

        a = ac->addAction(QStringLiteral("tools_uncomment"));
        a->setText(i18n("Unco&mment"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D));
        a->setWhatsThis(
            i18n("This command removes comments from the current line or a selected block of text.<br /><br />"
                 "The characters for single/multiple line comments are defined within the language's highlighting."));
        connect(a, SIGNAL(triggered(bool)), SLOT(uncomment()));

        a = ac->addAction(QStringLiteral("tools_toggle_comment"));
        a->setText(i18n("Toggle Comment"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_Slash));
        connect(a, SIGNAL(triggered(bool)), SLOT(toggleComment()));

        a = m_toggleWriteLock = new KToggleAction(i18n("&Read Only Mode"), this);
        a->setWhatsThis(i18n("Lock/unlock the document for writing"));
        a->setChecked(!doc()->isReadWrite());
        connect(a, SIGNAL(triggered(bool)), SLOT(toggleWriteLock()));
        ac->addAction(QStringLiteral("tools_toggle_write_lock"), a);

        a = ac->addAction(QStringLiteral("tools_uppercase"));
        a->setIcon(QIcon::fromTheme(QStringLiteral("format-text-uppercase")));
        a->setText(i18n("Uppercase"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_U));
        a->setWhatsThis(
            i18n("Convert the selection to uppercase, or the character to the "
                 "right of the cursor if no text is selected."));
        connect(a, SIGNAL(triggered(bool)), SLOT(uppercase()));

        a = ac->addAction(QStringLiteral("tools_lowercase"));
        a->setIcon(QIcon::fromTheme(QStringLiteral("format-text-lowercase")));
        a->setText(i18n("Lowercase"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_U));
        a->setWhatsThis(
            i18n("Convert the selection to lowercase, or the character to the "
                 "right of the cursor if no text is selected."));
        connect(a, SIGNAL(triggered(bool)), SLOT(lowercase()));

        a = ac->addAction(QStringLiteral("tools_capitalize"));
        a->setIcon(QIcon::fromTheme(QStringLiteral("format-text-capitalize")));
        a->setText(i18n("Capitalize"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_U));
        a->setWhatsThis(
            i18n("Capitalize the selection, or the word under the "
                 "cursor if no text is selected."));
        connect(a, SIGNAL(triggered(bool)), SLOT(capitalize()));

        a = ac->addAction(QStringLiteral("tools_join_lines"));
        a->setText(i18n("Join Lines"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_J));
        connect(a, SIGNAL(triggered(bool)), SLOT(joinLines()));

        a = ac->addAction(QStringLiteral("tools_invoke_code_completion"));
        a->setText(i18n("Invoke Code Completion"));
        a->setWhatsThis(i18n("Manually invoke command completion, usually by using a shortcut bound to this action."));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_Space));
        connect(a, SIGNAL(triggered(bool)), SLOT(userInvokedCompletion()));
    } else {
        m_cut->setEnabled(false);
        m_paste->setEnabled(false);
        m_pasteMenu->setEnabled(false);
        m_editUndo = nullptr;
        m_editRedo = nullptr;
    }

    a = ac->addAction(KStandardAction::Print, this, SLOT(print()));
    a->setWhatsThis(i18n("Print the current document."));

    a = ac->addAction(KStandardAction::PrintPreview, this, SLOT(printPreview()));
    a->setWhatsThis(i18n("Show print preview of current document"));

    a = ac->addAction(QStringLiteral("file_reload"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    a->setText(i18n("Reloa&d"));
    ac->setDefaultShortcuts(a, KStandardShortcut::reload());
    a->setWhatsThis(i18n("Reload the current document from disk."));
    connect(a, SIGNAL(triggered(bool)), SLOT(reloadFile()));

    a = ac->addAction(KStandardAction::SaveAs, m_doc, SLOT(documentSaveAs()));
    a->setWhatsThis(i18n("Save the current document to disk, with a name of your choice."));

    a = new KateViewEncodingAction(m_doc, this, i18n("Save As with Encoding..."), this, true /* special mode for save as */);
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-save-as")));
    ac->addAction(QStringLiteral("file_save_as_with_encoding"), a);

    a = ac->addAction(QStringLiteral("file_save_copy_as"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-save-as")));
    a->setText(i18n("Save &Copy As..."));
    a->setWhatsThis(i18n("Save a copy of the current document to disk."));
    connect(a, SIGNAL(triggered(bool)), m_doc, SLOT(documentSaveCopyAs()));

    a = ac->addAction(KStandardAction::GotoLine, this, SLOT(gotoLine()));
    a->setWhatsThis(i18n("This command opens a dialog and lets you choose a line that you want the cursor to move to."));

    a = ac->addAction(QStringLiteral("modified_line_up"));
    a->setText(i18n("Move to Previous Modified Line"));
    a->setWhatsThis(i18n("Move upwards to the previous modified line."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toPrevModifiedLine()));

    a = ac->addAction(QStringLiteral("modified_line_down"));
    a->setText(i18n("Move to Next Modified Line"));
    a->setWhatsThis(i18n("Move downwards to the next modified line."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toNextModifiedLine()));

    a = ac->addAction(QStringLiteral("set_confdlg"));
    a->setText(i18n("&Configure Editor..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("preferences-other")));
    a->setWhatsThis(i18n("Configure various aspects of this editor."));
    connect(a, SIGNAL(triggered(bool)), SLOT(slotConfigDialog()));

    m_modeAction = new KateModeMenu(i18n("&Mode"), this);
    ac->addAction(QStringLiteral("tools_mode"), m_modeAction);
    m_modeAction->setWhatsThis(i18n("Here you can choose which mode should be used for the current document. This will influence the highlighting and folding being used, for example."));
    m_modeAction->updateMenu(m_doc);

    KateHighlightingMenu *menu = new KateHighlightingMenu(i18n("&Highlighting"), this);
    ac->addAction(QStringLiteral("tools_highlighting"), menu);
    menu->setWhatsThis(i18n("Here you can choose how the current document should be highlighted."));
    menu->updateMenu(m_doc);

    KateViewSchemaAction *schemaMenu = new KateViewSchemaAction(i18n("&Schema"), this);
    ac->addAction(QStringLiteral("view_schemas"), schemaMenu);
    schemaMenu->updateMenu(this);

    // indentation menu
    KateViewIndentationAction *indentMenu = new KateViewIndentationAction(m_doc, i18n("&Indentation"), this);
    ac->addAction(QStringLiteral("tools_indentation"), indentMenu);

    m_selectAll = a = ac->addAction(KStandardAction::SelectAll, this, SLOT(selectAll()));
    a->setWhatsThis(i18n("Select the entire text of the current document."));

    m_deSelect = a = ac->addAction(KStandardAction::Deselect, this, SLOT(clearSelection()));
    a->setWhatsThis(i18n("If you have selected something within the current document, this will no longer be selected."));

    a = ac->addAction(QStringLiteral("view_inc_font_sizes"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("zoom-in")));
    a->setText(i18n("Enlarge Font"));
    ac->setDefaultShortcuts(a, KStandardShortcut::zoomIn());
    a->setWhatsThis(i18n("This increases the display font size."));
    connect(a, SIGNAL(triggered(bool)), m_viewInternal, SLOT(slotIncFontSizes()));

    a = ac->addAction(QStringLiteral("view_dec_font_sizes"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("zoom-out")));
    a->setText(i18n("Shrink Font"));
    ac->setDefaultShortcuts(a, KStandardShortcut::zoomOut());
    a->setWhatsThis(i18n("This decreases the display font size."));
    connect(a, SIGNAL(triggered(bool)), m_viewInternal, SLOT(slotDecFontSizes()));

    a = ac->addAction(QStringLiteral("view_reset_font_sizes"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("zoom-original")));
    a->setText(i18n("Reset Font Size"));
    ac->setDefaultShortcuts(a, KStandardShortcut::shortcut(KStandardShortcut::ActualSize));
    a->setWhatsThis(i18n("This resets the display font size."));
    connect(a, SIGNAL(triggered(bool)), m_viewInternal, SLOT(slotResetFontSizes()));

    a = m_toggleBlockSelection = new KToggleAction(i18n("Bl&ock Selection Mode"), this);
    ac->addAction(QStringLiteral("set_verticalSelect"), a);
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    a->setWhatsThis(i18n("This command allows switching between the normal (line based) selection mode and the block selection mode."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleBlockSelection()));

    a = ac->addAction(QStringLiteral("switch_next_input_mode"));
    a->setText(i18n("Switch to next Input Mode"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_V));
    a->setWhatsThis(i18n("Switch to the next input mode."));
    connect(a, SIGNAL(triggered(bool)), SLOT(cycleInputMode()));

    a = m_toggleInsert = new KToggleAction(i18n("Overwr&ite Mode"), this);
    ac->addAction(QStringLiteral("set_insert"), a);
    ac->setDefaultShortcut(a, QKeySequence(Qt::Key_Insert));
    a->setWhatsThis(i18n("Choose whether you want the text you type to be inserted or to overwrite existing text."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleInsert()));

    KToggleAction *toggleAction;
    a = m_toggleDynWrap = toggleAction = new KToggleAction(i18n("&Dynamic Word Wrap"), this);
    ac->addAction(QStringLiteral("view_dynamic_word_wrap"), a);
    ac->setDefaultShortcut(a, QKeySequence(Qt::Key_F10));
    a->setWhatsThis(
        i18n("If this option is checked, the text lines will be wrapped at the view border on the screen.<br /><br />"
             "This is only a view option, meaning the document will not changed."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleDynWordWrap()));

    a = m_setDynWrapIndicators = new KSelectAction(i18n("Dynamic Word Wrap Indicators"), this);
    ac->addAction(QStringLiteral("dynamic_word_wrap_indicators"), a);
    a->setWhatsThis(i18n("Choose when the Dynamic Word Wrap Indicators should be displayed"));
    connect(m_setDynWrapIndicators, SIGNAL(triggered(int)), this, SLOT(setDynWrapIndicators(int)));
    const QStringList list2 {i18n("&Off"), i18n("Follow &Line Numbers"), i18n("&Always On")};
    m_setDynWrapIndicators->setItems(list2);
    m_setDynWrapIndicators->setEnabled(m_toggleDynWrap->isChecked()); // only synced on real change, later

    a = toggleAction = new KToggleAction(i18n("Static Word Wrap"), this);
    ac->addAction(QStringLiteral("view_static_word_wrap"), a);
    a->setWhatsThis(i18n("If this option is checked, the text lines will be wrapped at the column defined in the editing properties."));
    connect(a, &KToggleAction::triggered, [=] {
        if (m_doc) {
            m_doc->setWordWrap(!m_doc->wordWrap());
        }
    });

    a = toggleAction = m_toggleWWMarker = new KToggleAction(i18n("Show Static &Word Wrap Marker"), this);
    ac->addAction(QStringLiteral("view_word_wrap_marker"), a);
    a->setWhatsThis(
        i18n("Show/hide the Word Wrap Marker, a vertical line drawn at the word "
             "wrap column as defined in the editing properties"));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleWWMarker()));

    a = toggleAction = m_toggleFoldingMarkers = new KToggleAction(i18n("Show Folding &Markers"), this);
    ac->addAction(QStringLiteral("view_folding_markers"), a);
    ac->setDefaultShortcut(a, QKeySequence(Qt::Key_F9));
    a->setWhatsThis(i18n("You can choose if the codefolding marks should be shown, if codefolding is possible."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleFoldingMarkers()));

    a = m_toggleIconBar = toggleAction = new KToggleAction(i18n("Show &Icon Border"), this);
    ac->addAction(QStringLiteral("view_border"), a);
    a->setWhatsThis(i18n("Show/hide the icon border.<br /><br />The icon border shows bookmark symbols, for instance."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleIconBorder()));

    a = toggleAction = m_toggleLineNumbers = new KToggleAction(i18n("Show &Line Numbers"), this);
    ac->addAction(QStringLiteral("view_line_numbers"), a);
    ac->setDefaultShortcut(a, QKeySequence(Qt::Key_F11));
    a->setWhatsThis(i18n("Show/hide the line numbers on the left hand side of the view."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleLineNumbersOn()));

    a = m_toggleScrollBarMarks = toggleAction = new KToggleAction(i18n("Show Scroll&bar Marks"), this);
    ac->addAction(QStringLiteral("view_scrollbar_marks"), a);
    a->setWhatsThis(i18n("Show/hide the marks on the vertical scrollbar.<br /><br />The marks show bookmarks, for instance."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMarks()));

    a = m_toggleScrollBarMiniMap = toggleAction = new KToggleAction(i18n("Show Scrollbar Mini-Map"), this);
    ac->addAction(QStringLiteral("view_scrollbar_minimap"), a);
    a->setWhatsThis(i18n("Show/hide the mini-map on the vertical scrollbar.<br /><br />The mini-map shows an overview of the whole document."));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMiniMap()));

    a = m_doc->autoReloadToggleAction();
    ac->addAction(QStringLiteral("view_auto_reload"), a);

    //   a = m_toggleScrollBarMiniMapAll = toggleAction = new KToggleAction(i18n("Show the whole document in the Mini-Map"), this);
    //   ac->addAction(QLatin1String("view_scrollbar_minimap_all"), a);
    //   a->setWhatsThis(i18n("Display the whole document in the mini-map.<br /><br />With this option set the whole document will be visible in the mini-map."));
    //   connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMiniMapAll()));
    //   connect(m_toggleScrollBarMiniMap, SIGNAL(triggered(bool)), m_toggleScrollBarMiniMapAll, SLOT(setEnabled(bool)));

    a = m_toggleNPSpaces = new KToggleAction(i18n("Show Non-Printable Spaces"), this);
    ac->addAction(QStringLiteral("view_non_printable_spaces"), a);
    a->setWhatsThis(i18n("Show/hide bounding box around non-printable spaces"));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleNPSpaces()));

    a = m_switchCmdLine = ac->addAction(QStringLiteral("switch_to_cmd_line"));
    a->setText(i18n("Switch to Command Line"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::Key_F7));
    a->setWhatsThis(i18n("Show/hide the command line on the bottom of the view."));
    connect(a, SIGNAL(triggered(bool)), SLOT(switchToCmdLine()));

    KActionMenu *am = new KActionMenu(i18n("Input Modes"), this);
    m_inputModeActions = new QActionGroup(am);
    ac->addAction(QStringLiteral("view_input_modes"), am);
    auto switchInputModeAction = ac->action(QStringLiteral("switch_next_input_mode"));
    am->addAction(switchInputModeAction);
    am->addSeparator();
    for (KateAbstractInputMode *mode : qAsConst(m_viewInternal->m_inputModes)) {
        a = new QAction(mode->viewInputModeHuman(), m_inputModeActions);
        am->addAction(a);
        a->setWhatsThis(i18n("Activate/deactivate %1", mode->viewInputModeHuman()));
        const InputMode im = mode->viewInputMode();
        a->setData(static_cast<int>(im));
        a->setCheckable(true);
        if (im == m_config->inputMode())
            a->setChecked(true);
        connect(a, SIGNAL(triggered()), SLOT(toggleInputMode()));
    }

    a = m_setEndOfLine = new KSelectAction(i18n("&End of Line"), this);
    ac->addAction(QStringLiteral("set_eol"), a);
    a->setWhatsThis(i18n("Choose which line endings should be used, when you save the document"));
    const QStringList list {i18nc("@item:inmenu End of Line", "&UNIX"), i18nc("@item:inmenu End of Line", "&Windows/DOS"), i18nc("@item:inmenu End of Line", "&Macintosh")};
    m_setEndOfLine->setItems(list);
    m_setEndOfLine->setCurrentItem(doc()->config()->eol());
    connect(m_setEndOfLine, SIGNAL(triggered(int)), this, SLOT(setEol(int)));

    a = m_addBom = new KToggleAction(i18n("Add &Byte Order Mark (BOM)"), this);
    m_addBom->setChecked(doc()->config()->bom());
    ac->addAction(QStringLiteral("add_bom"), a);
    a->setWhatsThis(i18n("Enable/disable adding of byte order marks for UTF-8/UTF-16 encoded files while saving"));
    connect(m_addBom, SIGNAL(triggered(bool)), this, SLOT(setAddBom(bool)));

    // encoding menu
    m_encodingAction = new KateViewEncodingAction(m_doc, this, i18n("E&ncoding"), this);
    ac->addAction(QStringLiteral("set_encoding"), m_encodingAction);

    a = ac->addAction(KStandardAction::Find, this, SLOT(find()));
    a->setWhatsThis(i18n("Look up the first occurrence of a piece of text or regular expression."));
    addAction(a);

    a = ac->addAction(QStringLiteral("edit_find_selected"));
    a->setText(i18n("Find Selected"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_H));
    a->setWhatsThis(i18n("Finds next occurrence of selected text."));
    connect(a, SIGNAL(triggered(bool)), SLOT(findSelectedForwards()));

    a = ac->addAction(QStringLiteral("edit_find_selected_backwards"));
    a->setText(i18n("Find Selected Backwards"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_H));
    a->setWhatsThis(i18n("Finds previous occurrence of selected text."));
    connect(a, SIGNAL(triggered(bool)), SLOT(findSelectedBackwards()));

    a = ac->addAction(KStandardAction::FindNext, this, SLOT(findNext()));
    a->setWhatsThis(i18n("Look up the next occurrence of the search phrase."));
    addAction(a);

    a = ac->addAction(KStandardAction::FindPrev, QStringLiteral("edit_find_prev"), this, SLOT(findPrevious()));
    a->setWhatsThis(i18n("Look up the previous occurrence of the search phrase."));
    addAction(a);

    a = ac->addAction(KStandardAction::Replace, this, SLOT(replace()));
    a->setWhatsThis(i18n("Look up a piece of text or regular expression and replace the result with some given text."));

    m_spell->createActions(ac);
    m_toggleOnTheFlySpellCheck = new KToggleAction(i18n("Automatic Spell Checking"), this);
    m_toggleOnTheFlySpellCheck->setWhatsThis(i18n("Enable/disable automatic spell checking"));
    connect(m_toggleOnTheFlySpellCheck, SIGNAL(triggered(bool)), SLOT(toggleOnTheFlySpellCheck(bool)));
    ac->addAction(QStringLiteral("tools_toggle_automatic_spell_checking"), m_toggleOnTheFlySpellCheck);
    ac->setDefaultShortcut(m_toggleOnTheFlySpellCheck, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));

    a = ac->addAction(QStringLiteral("tools_change_dictionary"));
    a->setText(i18n("Change Dictionary..."));
    a->setWhatsThis(i18n("Change the dictionary that is used for spell checking."));
    connect(a, SIGNAL(triggered()), SLOT(changeDictionary()));

    a = ac->addAction(QStringLiteral("tools_clear_dictionary_ranges"));
    a->setText(i18n("Clear Dictionary Ranges"));
    a->setEnabled(false);
    a->setWhatsThis(i18n("Remove all the separate dictionary ranges that were set for spell checking."));
    connect(a, SIGNAL(triggered()), m_doc, SLOT(clearDictionaryRanges()));
    connect(m_doc, SIGNAL(dictionaryRangesPresent(bool)), a, SLOT(setEnabled(bool)));

    m_copyHtmlAction = ac->addAction(QStringLiteral("edit_copy_html"), this, SLOT(exportHtmlToClipboard()));
    m_copyHtmlAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    m_copyHtmlAction->setText(i18n("Copy as &HTML"));
    m_copyHtmlAction->setWhatsThis(i18n("Use this command to copy the currently selected text as HTML to the system clipboard."));

    a = ac->addAction(QStringLiteral("file_export_html"), this, SLOT(exportHtmlToFile()));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-export")));
    a->setText(i18n("E&xport as HTML..."));
    a->setWhatsThis(
        i18n("This command allows you to export the current document"
             " with all highlighting information into a HTML document."));

    m_spellingMenu->createActions(ac);

    m_bookmarks->createActions(ac);

    slotSelectionChanged();

    // Now setup the editing actions before adding the associated
    // widget and setting the shortcut context
    setupEditActions();
    setupCodeFolding();
    slotClipboardHistoryChanged();

    ac->addAssociatedWidget(m_viewInternal);

    const auto actions = ac->actions();
    for (QAction *action : actions) {
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }

    connect(this, SIGNAL(selectionChanged(KTextEditor::View *)), this, SLOT(slotSelectionChanged()));
}

void KTextEditor::ViewPrivate::slotConfigDialog()
{
    // invoke config dialog, will auto-save configuration to katepartrc
    KTextEditor::EditorPrivate::self()->configDialog(this);
}

void KTextEditor::ViewPrivate::setupEditActions()
{
    // If you add an editing action to this
    // function make sure to include the line
    // m_editActions << a after creating the action
    KActionCollection *ac = actionCollection();

    QAction *a = ac->addAction(QStringLiteral("word_left"));
    a->setText(i18n("Move Word Left"));
    ac->setDefaultShortcuts(a, KStandardShortcut::backwardWord());
    connect(a, SIGNAL(triggered(bool)), SLOT(wordLeft()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_char_left"));
    a->setText(i18n("Select Character Left"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::Key_Left));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftCursorLeft()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_word_left"));
    a->setText(i18n("Select Word Left"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Left));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftWordLeft()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("word_right"));
    a->setText(i18n("Move Word Right"));
    ac->setDefaultShortcuts(a, KStandardShortcut::forwardWord());
    connect(a, SIGNAL(triggered(bool)), SLOT(wordRight()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_char_right"));
    a->setText(i18n("Select Character Right"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::Key_Right));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftCursorRight()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_word_right"));
    a->setText(i18n("Select Word Right"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Right));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftWordRight()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("beginning_of_line"));
    a->setText(i18n("Move to Beginning of Line"));
    ac->setDefaultShortcuts(a, KStandardShortcut::beginningOfLine());
    connect(a, SIGNAL(triggered(bool)), SLOT(home()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("beginning_of_document"));
    a->setText(i18n("Move to Beginning of Document"));
    ac->setDefaultShortcuts(a, KStandardShortcut::begin());
    connect(a, SIGNAL(triggered(bool)), SLOT(top()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_beginning_of_line"));
    a->setText(i18n("Select to Beginning of Line"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::Key_Home));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftHome()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_beginning_of_document"));
    a->setText(i18n("Select to Beginning of Document"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Home));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftTop()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("end_of_line"));
    a->setText(i18n("Move to End of Line"));
    ac->setDefaultShortcuts(a, KStandardShortcut::endOfLine());
    connect(a, SIGNAL(triggered(bool)), SLOT(end()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("end_of_document"));
    a->setText(i18n("Move to End of Document"));
    ac->setDefaultShortcuts(a, KStandardShortcut::end());
    connect(a, SIGNAL(triggered(bool)), SLOT(bottom()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_end_of_line"));
    a->setText(i18n("Select to End of Line"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::Key_End));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftEnd()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_end_of_document"));
    a->setText(i18n("Select to End of Document"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_End));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftBottom()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_line_up"));
    a->setText(i18n("Select to Previous Line"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::Key_Up));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftUp()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("scroll_line_up"));
    a->setText(i18n("Scroll Line Up"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_Up));
    connect(a, SIGNAL(triggered(bool)), SLOT(scrollUp()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("move_line_down"));
    a->setText(i18n("Move to Next Line"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::Key_Down));
    connect(a, SIGNAL(triggered(bool)), SLOT(down()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("move_line_up"));
    a->setText(i18n("Move to Previous Line"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::Key_Up));
    connect(a, SIGNAL(triggered(bool)), SLOT(up()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("move_cursor_right"));
    a->setText(i18n("Move Cursor Right"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::Key_Right));
    connect(a, SIGNAL(triggered(bool)), SLOT(cursorRight()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("move_cursor_left"));
    a->setText(i18n("Move Cursor Left"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::Key_Left));
    connect(a, SIGNAL(triggered(bool)), SLOT(cursorLeft()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_line_down"));
    a->setText(i18n("Select to Next Line"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::Key_Down));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftDown()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("scroll_line_down"));
    a->setText(i18n("Scroll Line Down"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_Down));
    connect(a, SIGNAL(triggered(bool)), SLOT(scrollDown()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("scroll_page_up"));
    a->setText(i18n("Scroll Page Up"));
    ac->setDefaultShortcuts(a, KStandardShortcut::prior());
    connect(a, SIGNAL(triggered(bool)), SLOT(pageUp()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_page_up"));
    a->setText(i18n("Select Page Up"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::Key_PageUp));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftPageUp()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("move_top_of_view"));
    a->setText(i18n("Move to Top of View"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::ALT + Qt::Key_Home));
    connect(a, SIGNAL(triggered(bool)), SLOT(topOfView()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_top_of_view"));
    a->setText(i18n("Select to Top of View"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Home));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftTopOfView()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("scroll_page_down"));
    a->setText(i18n("Scroll Page Down"));
    ac->setDefaultShortcuts(a, KStandardShortcut::next());
    connect(a, SIGNAL(triggered(bool)), SLOT(pageDown()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_page_down"));
    a->setText(i18n("Select Page Down"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::Key_PageDown));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftPageDown()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("move_bottom_of_view"));
    a->setText(i18n("Move to Bottom of View"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::ALT + Qt::Key_End));
    connect(a, SIGNAL(triggered(bool)), SLOT(bottomOfView()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("select_bottom_of_view"));
    a->setText(i18n("Select to Bottom of View"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_End));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftBottomOfView()));
    m_editActions << a;

    a = ac->addAction(QStringLiteral("to_matching_bracket"));
    a->setText(i18n("Move to Matching Bracket"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_6));
    connect(a, SIGNAL(triggered(bool)), SLOT(toMatchingBracket()));
    // m_editActions << a;

    a = ac->addAction(QStringLiteral("select_matching_bracket"));
    a->setText(i18n("Select to Matching Bracket"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_6));
    connect(a, SIGNAL(triggered(bool)), SLOT(shiftToMatchingBracket()));
    // m_editActions << a;

    // anders: shortcuts doing any changes should not be created in read-only mode
    if (!doc()->readOnly()) {
        a = ac->addAction(QStringLiteral("transpose_char"));
        a->setText(i18n("Transpose Characters"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_T));
        connect(a, SIGNAL(triggered(bool)), SLOT(transpose()));
        m_editActions << a;

        a = ac->addAction(QStringLiteral("delete_line"));
        a->setText(i18n("Delete Line"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_K));
        connect(a, SIGNAL(triggered(bool)), SLOT(killLine()));
        m_editActions << a;

        a = ac->addAction(QStringLiteral("delete_word_left"));
        a->setText(i18n("Delete Word Left"));
        ac->setDefaultShortcuts(a, KStandardShortcut::deleteWordBack());
        connect(a, SIGNAL(triggered(bool)), SLOT(deleteWordLeft()));
        m_editActions << a;

        a = ac->addAction(QStringLiteral("delete_word_right"));
        a->setText(i18n("Delete Word Right"));
        ac->setDefaultShortcuts(a, KStandardShortcut::deleteWordForward());
        connect(a, SIGNAL(triggered(bool)), SLOT(deleteWordRight()));
        m_editActions << a;

        a = ac->addAction(QStringLiteral("delete_next_character"));
        a->setText(i18n("Delete Next Character"));
        ac->setDefaultShortcut(a, QKeySequence(Qt::Key_Delete));
        connect(a, SIGNAL(triggered(bool)), SLOT(keyDelete()));
        m_editActions << a;

        a = ac->addAction(QStringLiteral("backspace"));
        a->setText(i18n("Backspace"));
        QList<QKeySequence> scuts;
        scuts << QKeySequence(Qt::Key_Backspace) << QKeySequence(Qt::SHIFT + Qt::Key_Backspace);
        ac->setDefaultShortcuts(a, scuts);
        connect(a, SIGNAL(triggered(bool)), SLOT(backspace()));
        m_editActions << a;

        a = ac->addAction(QStringLiteral("insert_tabulator"));
        a->setText(i18n("Insert Tab"));
        connect(a, SIGNAL(triggered(bool)), SLOT(insertTab()));
        m_editActions << a;

        a = ac->addAction(QStringLiteral("smart_newline"));
        a->setText(i18n("Insert Smart Newline"));
        a->setWhatsThis(i18n("Insert newline including leading characters of the current line which are not letters or numbers."));
        scuts.clear();
        scuts << QKeySequence(Qt::SHIFT + Qt::Key_Return) << QKeySequence(Qt::SHIFT + Qt::Key_Enter);
        ac->setDefaultShortcuts(a, scuts);
        connect(a, SIGNAL(triggered(bool)), SLOT(smartNewline()));
        m_editActions << a;

        a = ac->addAction(QStringLiteral("no_indent_newline"));
        a->setText(i18n("Insert a non-indented Newline"));
        a->setWhatsThis(i18n("Insert a new line without indentation, regardless of indentation settings."));
        scuts.clear();
        scuts << QKeySequence(Qt::CTRL + Qt::Key_Return) << QKeySequence(Qt::CTRL + Qt::Key_Enter);
        ac->setDefaultShortcuts(a, scuts);
        connect(a, SIGNAL(triggered(bool)), SLOT(noIndentNewline()));
        m_editActions << a;

        a = ac->addAction(QStringLiteral("tools_indent"));
        a->setIcon(QIcon::fromTheme(QStringLiteral("format-indent-more")));
        a->setText(i18n("&Indent"));
        a->setWhatsThis(
            i18n("Use this to indent a selected block of text.<br /><br />"
                 "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::Key_I));
        connect(a, SIGNAL(triggered(bool)), SLOT(indent()));

        a = ac->addAction(QStringLiteral("tools_unindent"));
        a->setIcon(QIcon::fromTheme(QStringLiteral("format-indent-less")));
        a->setText(i18n("&Unindent"));
        a->setWhatsThis(i18n("Use this to unindent a selected block of text."));
        ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
        connect(a, SIGNAL(triggered(bool)), SLOT(unIndent()));
    }

    if (hasFocus()) {
        slotGotFocus();
    } else {
        slotLostFocus();
    }
}

void KTextEditor::ViewPrivate::setupCodeFolding()
{
    KActionCollection *ac = this->actionCollection();
    QAction *a;

    a = ac->addAction(QStringLiteral("folding_toplevel"));
    a->setText(i18n("Fold Toplevel Nodes"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Minus));
    connect(a, SIGNAL(triggered(bool)), SLOT(slotFoldToplevelNodes()));

    a = ac->addAction(QStringLiteral("folding_expandtoplevel"));
    a->setText(i18n("Unfold Toplevel Nodes"));
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Plus));
    connect(a, SIGNAL(triggered(bool)), SLOT(slotExpandToplevelNodes()));

    /*a = ac->addAction(QLatin1String("folding_expandall"));
    a->setText(i18n("Unfold All Nodes"));
    connect(a, SIGNAL(triggered(bool)), doc()->foldingTree(), SLOT(expandAll()));

    a = ac->addAction(QLatin1String("folding_collapse_dsComment"));
    a->setText(i18n("Fold Multiline Comments"));
    connect(a, SIGNAL(triggered(bool)), doc()->foldingTree(), SLOT(collapseAll_dsComments()));
    */
    a = ac->addAction(QStringLiteral("folding_toggle_current"));
    a->setText(i18n("Toggle Current Node"));
    connect(a, &QAction::triggered, this, &KTextEditor::ViewPrivate::slotToggleFolding);

    a = ac->addAction(QStringLiteral("folding_toggle_in_current"));
    a->setText(i18n("Toggle Contained Nodes"));
    connect(a, &QAction::triggered, this, &KTextEditor::ViewPrivate::slotToggleFoldingsInRange);
}

void KTextEditor::ViewPrivate::slotFoldToplevelNodes()
{
    for (int line = 0; line < doc()->lines(); ++line) {
        if (textFolding().isLineVisible(line)) {
            foldLine(line);
        }
    }
}

void KTextEditor::ViewPrivate::slotExpandToplevelNodes()
{
    const auto topLevelRanges(textFolding().foldingRangesForParentRange());
    for (const auto &range : topLevelRanges) {
        textFolding().unfoldRange(range.first);
    }
}

void KTextEditor::ViewPrivate::slotToggleFolding()
{
    int line = cursorPosition().line();
    bool actionDone = false;
    while (!actionDone && (line > -1)) {
        actionDone = unfoldLine(line);
        if (!actionDone) {
            actionDone = foldLine(line--).isValid();
        }
    }
}

void KTextEditor::ViewPrivate::slotToggleFoldingsInRange()
{
    int line = cursorPosition().line();
    while (!toggleFoldingsInRange(line) && (line > -1)) {
        --line;
    }
}

KTextEditor::Range KTextEditor::ViewPrivate::foldLine(int line)
{
    KTextEditor::Range foldingRange = doc()->buffer().computeFoldingRangeForStartLine(line);
    if (!foldingRange.isValid()) {
        return foldingRange;
    }

    // Ensure not to fold the end marker to avoid a deceptive look, but only on token based folding
    // ensure we don't compute an invalid line by moving outside of the foldingRange range by checking onSingleLine(), see bug 417890
    Kate::TextLine startTextLine = doc()->buffer().plainLine(line);
    if (!startTextLine->markedAsFoldingStartIndentation() && !foldingRange.onSingleLine()) {
        const int adjustedLine = foldingRange.end().line() - 1;
        foldingRange.setEnd(KTextEditor::Cursor(adjustedLine, doc()->buffer().plainLine(adjustedLine)->length()));
    }

    // Don't try to fold a single line, which can happens due to adjustment above
    // FIXME Avoid to offer such a folding marker
    if (!foldingRange.onSingleLine()) {
        textFolding().newFoldingRange(foldingRange, Kate::TextFolding::Folded);
    }

    return foldingRange;
}

bool KTextEditor::ViewPrivate::unfoldLine(int line)
{
    bool actionDone = false;
    const KTextEditor::Cursor currentCursor = cursorPosition();

    // ask the folding info for this line, if any folds are around!
    // auto = QVector<QPair<qint64, Kate::TextFolding::FoldingRangeFlags>>
    auto startingRanges = textFolding().foldingRangesStartingOnLine(line);
    for (int i = 0; i < startingRanges.size() && !actionDone; ++i) {
        // Avoid jumping view in case of a big unfold and ensure nice highlight of folding marker
        setCursorPosition(textFolding().foldingRange(startingRanges[i].first).start());

        actionDone |= textFolding().unfoldRange(startingRanges[i].first);
    }

    if (!actionDone) {
        // Nothing unfolded? Restore old cursor position!
        setCursorPosition(currentCursor);
    }

    return actionDone;
}

bool KTextEditor::ViewPrivate::toggleFoldingOfLine(int line)
{
    bool actionDone = unfoldLine(line);
    if (!actionDone) {
        actionDone = foldLine(line).isValid();
    }

    return actionDone;
}

bool KTextEditor::ViewPrivate::toggleFoldingsInRange(int line)
{
    KTextEditor::Range foldingRange = doc()->buffer().computeFoldingRangeForStartLine(line);
    if (!foldingRange.isValid()) {
        // Either line is not valid or there is no start range
        return false;
    }

    bool actionDone = false; // Track success
    const KTextEditor::Cursor currentCursor = cursorPosition();

    // Don't be too eager but obliging! Only toggle containing ranges which are
    // visible -> Be done when the range is folded
    actionDone |= unfoldLine(line);

    if (!actionDone) {
        // Unfold all in range, but not the range itself
        for (int ln = foldingRange.start().line() + 1; ln < foldingRange.end().line(); ++ln) {
            actionDone |= unfoldLine(ln);
        }

        if (actionDone) {
            // In most cases we want now a not moved cursor
            setCursorPosition(currentCursor);
        }
    }

    if (!actionDone) {
        // Fold all in range, but not the range itself
        for (int ln = foldingRange.start().line() + 1; ln < foldingRange.end().line(); ++ln) {
            KTextEditor::Range fr = foldLine(ln);
            if (fr.isValid()) {
                // qMax to avoid infinite loop in case of range without content
                ln = qMax(ln, fr.end().line() - 1);
                actionDone = true;
            }
        }
    }

    if (!actionDone) {
        // At this point was an unfolded range clicked which contains no "childs"
        // We assume the user want to fold it by the wrong button, be obliging!
        actionDone |= foldLine(line).isValid();
    }

    // At this point we should be always true
    return actionDone;
}

KTextEditor::View::ViewMode KTextEditor::ViewPrivate::viewMode() const
{
    return currentInputMode()->viewMode();
}

QString KTextEditor::ViewPrivate::viewModeHuman() const
{
    QString currentMode = currentInputMode()->viewModeHuman();

    /**
     * append read-only if needed
     */
    if (!doc()->isReadWrite()) {
        currentMode = i18n("(R/O) %1", currentMode);
    }

    /**
     * return full mode
     */
    return currentMode;
}

KTextEditor::View::InputMode KTextEditor::ViewPrivate::viewInputMode() const
{
    return currentInputMode()->viewInputMode();
}

QString KTextEditor::ViewPrivate::viewInputModeHuman() const
{
    return currentInputMode()->viewInputModeHuman();
}

void KTextEditor::ViewPrivate::setInputMode(KTextEditor::View::InputMode mode)
{
    if (currentInputMode()->viewInputMode() == mode) {
        return;
    }

    if (!m_viewInternal->m_inputModes.contains(mode)) {
        return;
    }

    m_viewInternal->m_currentInputMode->deactivate();
    m_viewInternal->m_currentInputMode = m_viewInternal->m_inputModes[mode];
    m_viewInternal->m_currentInputMode->activate();

    config()->setValue(KateViewConfig::InputMode, mode); // TODO: this could be called from read config procedure, so it's not a good idea to set a specific view mode here

    /* small duplication, but need to do this if not toggled by action */
    const auto inputModeActions = m_inputModeActions->actions();
    for (QAction *action : inputModeActions) {
        if (static_cast<InputMode>(action->data().toInt()) == mode) {
            action->setChecked(true);
            break;
        }
    }

    /* inform the rest of the system about the change */
    emit viewInputModeChanged(this, mode);
    emit viewModeChanged(this, viewMode());
}

void KTextEditor::ViewPrivate::slotDocumentAboutToReload()
{
    if (doc()->isAutoReload()) {
        const int lastVisibleLine = m_viewInternal->endLine();
        const int currentLine = cursorPosition().line();
        m_gotoBottomAfterReload = (lastVisibleLine == currentLine) && (currentLine == doc()->lastLine());
        if (!m_gotoBottomAfterReload) {
            // Ensure the view jumps not back when user scrolls around
            const int firstVisibleLine = 1 + lastVisibleLine - m_viewInternal->linesDisplayed();
            const int newLine = qBound(firstVisibleLine, currentLine, lastVisibleLine);
            setCursorPositionVisual(KTextEditor::Cursor(newLine, cursorPosition().column()));
        }
    } else {
        m_gotoBottomAfterReload = false;
    }
}

void KTextEditor::ViewPrivate::slotDocumentReloaded()
{
    if (m_gotoBottomAfterReload) {
        bottom();
    }
}

void KTextEditor::ViewPrivate::slotGotFocus()
{
    // qCDebug(LOG_KTE) << "KTextEditor::ViewPrivate::slotGotFocus";
    currentInputMode()->gotFocus();

    /**
     *  update current view and scrollbars
     *  it is needed for styles that implement different frame and scrollbar
     * rendering when focused
     */
    update();
    if (m_viewInternal->m_lineScroll->isVisible()) {
        m_viewInternal->m_lineScroll->update();
    }

    if (m_viewInternal->m_columnScroll->isVisible()) {
        m_viewInternal->m_columnScroll->update();
    }

    emit focusIn(this);
}

void KTextEditor::ViewPrivate::slotLostFocus()
{
    // qCDebug(LOG_KTE) << "KTextEditor::ViewPrivate::slotLostFocus";
    currentInputMode()->lostFocus();

    /**
     *  update current view and scrollbars
     *  it is needed for styles that implement different frame and scrollbar
     * rendering when focused
     */
    update();
    if (m_viewInternal->m_lineScroll->isVisible()) {
        m_viewInternal->m_lineScroll->update();
    }

    if (m_viewInternal->m_columnScroll->isVisible()) {
        m_viewInternal->m_columnScroll->update();
    }

    emit focusOut(this);
}

void KTextEditor::ViewPrivate::setDynWrapIndicators(int mode)
{
    config()->setValue(KateViewConfig::DynWordWrapIndicators, mode);
}

bool KTextEditor::ViewPrivate::isOverwriteMode() const
{
    return doc()->config()->ovr();
}

void KTextEditor::ViewPrivate::reloadFile()
{
    // bookmarks and cursor positions are temporarily saved by the document
    doc()->documentReload();
}

void KTextEditor::ViewPrivate::slotReadWriteChanged()
{
    if (m_toggleWriteLock) {
        m_toggleWriteLock->setChecked(!doc()->isReadWrite());
    }

    m_cut->setEnabled(doc()->isReadWrite() && (selection() || m_config->smartCopyCut()));
    m_paste->setEnabled(doc()->isReadWrite());
    m_pasteMenu->setEnabled(doc()->isReadWrite() && !KTextEditor::EditorPrivate::self()->clipboardHistory().isEmpty());
    m_setEndOfLine->setEnabled(doc()->isReadWrite());

    static const auto l = {QStringLiteral("edit_replace"),
                           QStringLiteral("tools_spelling"),
                           QStringLiteral("tools_indent"),
                           QStringLiteral("tools_unindent"),
                           QStringLiteral("tools_cleanIndent"),
                           QStringLiteral("tools_align"),
                           QStringLiteral("tools_comment"),
                           QStringLiteral("tools_uncomment"),
                           QStringLiteral("tools_toggle_comment"),
                           QStringLiteral("tools_uppercase"),
                           QStringLiteral("tools_lowercase"),
                           QStringLiteral("tools_capitalize"),
                           QStringLiteral("tools_join_lines"),
                           QStringLiteral("tools_apply_wordwrap"),
                           QStringLiteral("tools_spelling_from_cursor"),
                           QStringLiteral("tools_spelling_selection")};

    for (const auto &action : l) {
        QAction *a = actionCollection()->action(action);
        if (a) {
            a->setEnabled(doc()->isReadWrite());
        }
    }
    slotUpdateUndo();

    currentInputMode()->readWriteChanged(doc()->isReadWrite());

    // => view mode changed
    emit viewModeChanged(this, viewMode());
    emit viewInputModeChanged(this, viewInputMode());
}

void KTextEditor::ViewPrivate::slotClipboardHistoryChanged()
{
    m_pasteMenu->setEnabled(doc()->isReadWrite() && !KTextEditor::EditorPrivate::self()->clipboardHistory().isEmpty());
}

void KTextEditor::ViewPrivate::slotUpdateUndo()
{
    if (doc()->readOnly()) {
        return;
    }

    m_editUndo->setEnabled(doc()->isReadWrite() && doc()->undoCount() > 0);
    m_editRedo->setEnabled(doc()->isReadWrite() && doc()->redoCount() > 0);
}

bool KTextEditor::ViewPrivate::setCursorPositionInternal(const KTextEditor::Cursor &position, uint tabwidth, bool calledExternally)
{
    Kate::TextLine l = doc()->kateTextLine(position.line());

    if (!l) {
        return false;
    }

    QString line_str = doc()->line(position.line());

    int x = 0;
    int z = 0;
    for (; z < line_str.length() && z < position.column(); z++) {
        if (line_str[z] == QLatin1Char('\t')) {
            x += tabwidth - (x % tabwidth);
        } else {
            x++;
        }
    }

    if (blockSelection())
        if (z < position.column()) {
            x += position.column() - z;
        }

    m_viewInternal->updateCursor(KTextEditor::Cursor(position.line(), x), false, calledExternally /* force center for external calls, see bug 408418 */, calledExternally);

    return true;
}

void KTextEditor::ViewPrivate::toggleInsert()
{
    doc()->config()->setOvr(!doc()->config()->ovr());
    m_toggleInsert->setChecked(isOverwriteMode());

    emit viewModeChanged(this, viewMode());
    emit viewInputModeChanged(this, viewInputMode());
}

void KTextEditor::ViewPrivate::slotSaveCanceled(const QString &error)
{
    if (!error.isEmpty()) { // happens when canceling a job
        KMessageBox::error(this, error);
    }
}

void KTextEditor::ViewPrivate::gotoLine()
{
    gotoBar()->updateData();
    bottomViewBar()->showBarWidget(gotoBar());
}

void KTextEditor::ViewPrivate::changeDictionary()
{
    dictionaryBar()->updateData();
    bottomViewBar()->showBarWidget(dictionaryBar());
}

void KTextEditor::ViewPrivate::joinLines()
{
    int first = selectionRange().start().line();
    int last = selectionRange().end().line();
    // int left = doc()->line( last ).length() - doc()->selEndCol();
    if (first == last) {
        first = cursorPosition().line();
        last = first + 1;
    }
    doc()->joinLines(first, last);
}

void KTextEditor::ViewPrivate::readSessionConfig(const KConfigGroup &config, const QSet<QString> &flags)
{
    Q_UNUSED(flags)

    // cursor position
    setCursorPositionInternal(KTextEditor::Cursor(config.readEntry("CursorLine", 0), config.readEntry("CursorColumn", 0)));

    m_config->setDynWordWrap(config.readEntry("Dynamic Word Wrap", false));

    // restore text folding
    m_savedFoldingState = QJsonDocument::fromJson(config.readEntry("TextFolding", QByteArray()));
    applyFoldingState();

    for (KateAbstractInputMode *mode : qAsConst(m_viewInternal->m_inputModes)) {
        mode->readSessionConfig(config);
    }
}

void KTextEditor::ViewPrivate::writeSessionConfig(KConfigGroup &config, const QSet<QString> &flags)
{
    Q_UNUSED(flags)

    // cursor position
    config.writeEntry("CursorLine", cursorPosition().line());
    config.writeEntry("CursorColumn", cursorPosition().column());

    config.writeEntry("Dynamic Word Wrap", m_config->dynWordWrap());

    // save text folding state
    saveFoldingState();
    config.writeEntry("TextFolding", m_savedFoldingState.toJson(QJsonDocument::Compact));
    m_savedFoldingState = QJsonDocument();

    for (KateAbstractInputMode *mode : qAsConst(m_viewInternal->m_inputModes)) {
        mode->writeSessionConfig(config);
    }
}

int KTextEditor::ViewPrivate::getEol() const
{
    return doc()->config()->eol();
}

void KTextEditor::ViewPrivate::setEol(int eol)
{
    if (!doc()->isReadWrite()) {
        return;
    }

    if (m_updatingDocumentConfig) {
        return;
    }

    if (eol != doc()->config()->eol()) {
        doc()->setModified(true); // mark modified (bug #143120)
        doc()->config()->setEol(eol);
    }
}

void KTextEditor::ViewPrivate::setAddBom(bool enabled)
{
    if (!doc()->isReadWrite()) {
        return;
    }

    if (m_updatingDocumentConfig) {
        return;
    }

    doc()->config()->setBom(enabled);
    doc()->bomSetByUser();
}

void KTextEditor::ViewPrivate::setIconBorder(bool enable)
{
    config()->setValue(KateViewConfig::ShowIconBar, enable);
}

void KTextEditor::ViewPrivate::toggleIconBorder()
{
    config()->setValue(KateViewConfig::ShowIconBar, !config()->iconBar());
}

void KTextEditor::ViewPrivate::setLineNumbersOn(bool enable)
{
    config()->setValue(KateViewConfig::ShowLineNumbers, enable);
}

void KTextEditor::ViewPrivate::toggleLineNumbersOn()
{
    config()->setValue(KateViewConfig::ShowLineNumbers, !config()->lineNumbers());
}

void KTextEditor::ViewPrivate::setScrollBarMarks(bool enable)
{
    config()->setValue(KateViewConfig::ShowScrollBarMarks, enable);
}

void KTextEditor::ViewPrivate::toggleScrollBarMarks()
{
    config()->setValue(KateViewConfig::ShowScrollBarMarks, !config()->scrollBarMarks());
}

void KTextEditor::ViewPrivate::setScrollBarMiniMap(bool enable)
{
    config()->setValue(KateViewConfig::ShowScrollBarMiniMap, enable);
}

void KTextEditor::ViewPrivate::toggleScrollBarMiniMap()
{
    config()->setValue(KateViewConfig::ShowScrollBarMiniMap, !config()->scrollBarMiniMap());
}

void KTextEditor::ViewPrivate::setScrollBarMiniMapAll(bool enable)
{
    config()->setValue(KateViewConfig::ShowScrollBarMiniMapAll, enable);
}

void KTextEditor::ViewPrivate::toggleScrollBarMiniMapAll()
{
    config()->setValue(KateViewConfig::ShowScrollBarMiniMapAll, !config()->scrollBarMiniMapAll());
}

void KTextEditor::ViewPrivate::setScrollBarMiniMapWidth(int width)
{
    config()->setValue(KateViewConfig::ScrollBarMiniMapWidth, width);
}

void KTextEditor::ViewPrivate::toggleDynWordWrap()
{
    config()->setDynWordWrap(!config()->dynWordWrap());
}

void KTextEditor::ViewPrivate::toggleWWMarker()
{
    m_renderer->config()->setWordWrapMarker(!m_renderer->config()->wordWrapMarker());
}

void KTextEditor::ViewPrivate::toggleNPSpaces()
{
    m_renderer->setShowNonPrintableSpaces(!m_renderer->showNonPrintableSpaces());
    m_viewInternal->update(); // force redraw
}

void KTextEditor::ViewPrivate::toggleWordCount(bool on)
{
    config()->setShowWordCount(on);
}

void KTextEditor::ViewPrivate::setFoldingMarkersOn(bool enable)
{
    config()->setValue(KateViewConfig::ShowFoldingBar, enable);
}

void KTextEditor::ViewPrivate::toggleFoldingMarkers()
{
    config()->setValue(KateViewConfig::ShowFoldingBar, !config()->foldingBar());
}

bool KTextEditor::ViewPrivate::iconBorder()
{
    return m_viewInternal->m_leftBorder->iconBorderOn();
}

bool KTextEditor::ViewPrivate::lineNumbersOn()
{
    return m_viewInternal->m_leftBorder->lineNumbersOn();
}

bool KTextEditor::ViewPrivate::scrollBarMarks()
{
    return m_viewInternal->m_lineScroll->showMarks();
}

bool KTextEditor::ViewPrivate::scrollBarMiniMap()
{
    return m_viewInternal->m_lineScroll->showMiniMap();
}

int KTextEditor::ViewPrivate::dynWrapIndicators()
{
    return m_viewInternal->m_leftBorder->dynWrapIndicators();
}

bool KTextEditor::ViewPrivate::foldingMarkersOn()
{
    return m_viewInternal->m_leftBorder->foldingMarkersOn();
}

void KTextEditor::ViewPrivate::toggleWriteLock()
{
    doc()->setReadWrite(!doc()->isReadWrite());
}

void KTextEditor::ViewPrivate::registerTextHintProvider(KTextEditor::TextHintProvider *provider)
{
    m_viewInternal->registerTextHintProvider(provider);
}

void KTextEditor::ViewPrivate::unregisterTextHintProvider(KTextEditor::TextHintProvider *provider)
{
    m_viewInternal->unregisterTextHintProvider(provider);
}

void KTextEditor::ViewPrivate::setTextHintDelay(int delay)
{
    m_viewInternal->setTextHintDelay(delay);
}

int KTextEditor::ViewPrivate::textHintDelay() const
{
    return m_viewInternal->textHintDelay();
}

void KTextEditor::ViewPrivate::find()
{
    currentInputMode()->find();
}

void KTextEditor::ViewPrivate::findSelectedForwards()
{
    currentInputMode()->findSelectedForwards();
}

void KTextEditor::ViewPrivate::findSelectedBackwards()
{
    currentInputMode()->findSelectedBackwards();
}

void KTextEditor::ViewPrivate::replace()
{
    currentInputMode()->findReplace();
}

void KTextEditor::ViewPrivate::findNext()
{
    currentInputMode()->findNext();
}

void KTextEditor::ViewPrivate::findPrevious()
{
    currentInputMode()->findPrevious();
}

void KTextEditor::ViewPrivate::slotSelectionChanged()
{
    m_copy->setEnabled(selection() || m_config->smartCopyCut());
    m_deSelect->setEnabled(selection());
    m_copyHtmlAction->setEnabled(selection());

    // update highlighting of current selected word
    selectionChangedForHighlights();

    if (doc()->readOnly()) {
        return;
    }

    m_cut->setEnabled(selection() || m_config->smartCopyCut());
}

void KTextEditor::ViewPrivate::switchToCmdLine()
{
    currentInputMode()->activateCommandLine();
}

KateRenderer *KTextEditor::ViewPrivate::renderer()
{
    return m_renderer;
}

void KTextEditor::ViewPrivate::updateConfig()
{
    if (m_startingUp) {
        return;
    }

    // dyn. word wrap & markers
    if (m_hasWrap != config()->dynWordWrap()) {
        m_viewInternal->prepareForDynWrapChange();

        m_hasWrap = config()->dynWordWrap();

        m_viewInternal->dynWrapChanged();

        m_setDynWrapIndicators->setEnabled(config()->dynWordWrap());
        m_toggleDynWrap->setChecked(config()->dynWordWrap());
    }

    m_viewInternal->m_leftBorder->setDynWrapIndicators(config()->dynWordWrapIndicators());
    m_setDynWrapIndicators->setCurrentItem(config()->dynWordWrapIndicators());

    // line numbers
    m_viewInternal->m_leftBorder->setLineNumbersOn(config()->lineNumbers());
    m_toggleLineNumbers->setChecked(config()->lineNumbers());

    // icon bar
    m_viewInternal->m_leftBorder->setIconBorderOn(config()->iconBar());
    m_toggleIconBar->setChecked(config()->iconBar());

    // scrollbar marks
    m_viewInternal->m_lineScroll->setShowMarks(config()->scrollBarMarks());
    m_toggleScrollBarMarks->setChecked(config()->scrollBarMarks());

    // scrollbar mini-map
    m_viewInternal->m_lineScroll->setShowMiniMap(config()->scrollBarMiniMap());
    m_toggleScrollBarMiniMap->setChecked(config()->scrollBarMiniMap());

    // scrollbar mini-map - (whole document)
    m_viewInternal->m_lineScroll->setMiniMapAll(config()->scrollBarMiniMapAll());
    // m_toggleScrollBarMiniMapAll->setChecked( config()->scrollBarMiniMapAll() );

    // scrollbar mini-map.width
    m_viewInternal->m_lineScroll->setMiniMapWidth(config()->scrollBarMiniMapWidth());

    // misc edit
    m_toggleBlockSelection->setChecked(blockSelection());
    m_toggleInsert->setChecked(isOverwriteMode());

    updateFoldingConfig();

    // bookmark
    m_bookmarks->setSorting((KateBookmarks::Sorting)config()->bookmarkSort());

    m_viewInternal->setAutoCenterLines(config()->autoCenterLines());

    for (KateAbstractInputMode *input : qAsConst(m_viewInternal->m_inputModes)) {
        input->updateConfig();
    }

    setInputMode(config()->inputMode());

    reflectOnTheFlySpellCheckStatus(doc()->isOnTheFlySpellCheckingEnabled());

    // register/unregister word completion...
    bool wc = config()->wordCompletion();
    if (wc != isCompletionModelRegistered(KTextEditor::EditorPrivate::self()->wordCompletionModel())) {
        if (wc)
            registerCompletionModel(KTextEditor::EditorPrivate::self()->wordCompletionModel());
        else
            unregisterCompletionModel(KTextEditor::EditorPrivate::self()->wordCompletionModel());
    }

    bool kc = config()->keywordCompletion();
    if (kc != isCompletionModelRegistered(KTextEditor::EditorPrivate::self()->keywordCompletionModel())) {
        if (kc)
            registerCompletionModel(KTextEditor::EditorPrivate::self()->keywordCompletionModel());
        else
            unregisterCompletionModel(KTextEditor::EditorPrivate::self()->keywordCompletionModel());
    }

    m_cut->setEnabled(doc()->isReadWrite() && (selection() || m_config->smartCopyCut()));
    m_copy->setEnabled(selection() || m_config->smartCopyCut());

    // if not disabled, update status bar
    if (m_statusBar) {
        m_statusBar->updateStatus();
    }

    // now redraw...
    m_viewInternal->cache()->clear();
    tagAll();
    updateView(true);

    emit configChanged();
}

void KTextEditor::ViewPrivate::updateDocumentConfig()
{
    if (m_startingUp) {
        return;
    }

    m_updatingDocumentConfig = true;

    m_setEndOfLine->setCurrentItem(doc()->config()->eol());

    m_addBom->setChecked(doc()->config()->bom());

    m_updatingDocumentConfig = false;

    // maybe block selection or wrap-cursor mode changed
    ensureCursorColumnValid();

    // first change this
    m_renderer->setTabWidth(doc()->config()->tabWidth());
    m_renderer->setIndentWidth(doc()->config()->indentationWidth());

    // now redraw...
    m_viewInternal->cache()->clear();
    tagAll();
    updateView(true);
}

void KTextEditor::ViewPrivate::updateRendererConfig()
{
    if (m_startingUp) {
        return;
    }

    m_toggleWWMarker->setChecked(m_renderer->config()->wordWrapMarker());

    m_viewInternal->updateBracketMarkAttributes();
    m_viewInternal->updateBracketMarks();

    // now redraw...
    m_viewInternal->cache()->clear();
    tagAll();
    m_viewInternal->updateView(true);

    // update the left border right, for example linenumbers
    m_viewInternal->m_leftBorder->updateFont();
    m_viewInternal->m_leftBorder->repaint();

    m_viewInternal->m_lineScroll->queuePixmapUpdate();

    currentInputMode()->updateRendererConfig();

    // @@ showIndentLines is not cached anymore.
    //  m_renderer->setShowIndentLines (m_renderer->config()->showIndentationLines());
    emit configChanged();
}

void KTextEditor::ViewPrivate::updateFoldingConfig()
{
    // folding bar
    m_viewInternal->m_leftBorder->setFoldingMarkersOn(config()->foldingBar());
    m_toggleFoldingMarkers->setChecked(config()->foldingBar());

    if (hasCommentInFirstLine(m_doc)) {
        if (config()->foldFirstLine() && !m_autoFoldedFirstLine) {
            foldLine(0);
            m_autoFoldedFirstLine = true;
        } else if (!config()->foldFirstLine() && m_autoFoldedFirstLine) {
            unfoldLine(0);
            m_autoFoldedFirstLine = false;
        }
    } else {
        m_autoFoldedFirstLine = false;
    }

#if 0
    // FIXME: FOLDING
    const QStringList l = {
          QStringLiteral("folding_toplevel")
        , QStringLiteral("folding_expandtoplevel")
        , QStringLiteral("folding_toggle_current")
        , QStringLiteral("folding_toggle_in_current")
    };

    QAction *a = 0;
    for (int z = 0; z < l.size(); z++)
        if ((a = actionCollection()->action(l[z].toAscii().constData()))) {
            a->setEnabled(doc()->highlight() && doc()->highlight()->allowsFolding());
        }
#endif
}

void KTextEditor::ViewPrivate::ensureCursorColumnValid()
{
    KTextEditor::Cursor c = m_viewInternal->cursorPosition();

    // make sure the cursor is valid:
    // - in block selection mode or if wrap cursor is off, the column is arbitrary
    // - otherwise: it's bounded by the line length
    if (!blockSelection() && wrapCursor() && (!c.isValid() || c.column() > doc()->lineLength(c.line()))) {
        c.setColumn(doc()->kateTextLine(cursorPosition().line())->length());
        setCursorPosition(c);
    }
}

// BEGIN EDIT STUFF
void KTextEditor::ViewPrivate::editStart()
{
    m_viewInternal->editStart();
}

void KTextEditor::ViewPrivate::editEnd(int editTagLineStart, int editTagLineEnd, bool tagFrom)
{
    m_viewInternal->editEnd(editTagLineStart, editTagLineEnd, tagFrom);
}

void KTextEditor::ViewPrivate::editSetCursor(const KTextEditor::Cursor &cursor)
{
    m_viewInternal->editSetCursor(cursor);
}
// END

// BEGIN TAG & CLEAR
bool KTextEditor::ViewPrivate::tagLine(const KTextEditor::Cursor &virtualCursor)
{
    return m_viewInternal->tagLine(virtualCursor);
}

bool KTextEditor::ViewPrivate::tagRange(const KTextEditor::Range &range, bool realLines)
{
    return m_viewInternal->tagRange(range, realLines);
}

bool KTextEditor::ViewPrivate::tagLines(int start, int end, bool realLines)
{
    return m_viewInternal->tagLines(start, end, realLines);
}

bool KTextEditor::ViewPrivate::tagLines(KTextEditor::Cursor start, KTextEditor::Cursor end, bool realCursors)
{
    return m_viewInternal->tagLines(start, end, realCursors);
}

void KTextEditor::ViewPrivate::tagAll()
{
    m_viewInternal->tagAll();
}

void KTextEditor::ViewPrivate::clear()
{
    m_viewInternal->clear();
}

void KTextEditor::ViewPrivate::repaintText(bool paintOnlyDirty)
{
    if (paintOnlyDirty) {
        m_viewInternal->updateDirty();
    } else {
        m_viewInternal->update();
    }
}

void KTextEditor::ViewPrivate::updateView(bool changed)
{
    // qCDebug(LOG_KTE) << "KTextEditor::ViewPrivate::updateView";

    m_viewInternal->updateView(changed);
    m_viewInternal->m_leftBorder->update();
}

// END

void KTextEditor::ViewPrivate::slotHlChanged()
{
    KateHighlighting *hl = doc()->highlight();
    bool ok(!hl->getCommentStart(0).isEmpty() || !hl->getCommentSingleLineStart(0).isEmpty());

    if (actionCollection()->action(QStringLiteral("tools_comment"))) {
        actionCollection()->action(QStringLiteral("tools_comment"))->setEnabled(ok);
    }

    if (actionCollection()->action(QStringLiteral("tools_uncomment"))) {
        actionCollection()->action(QStringLiteral("tools_uncomment"))->setEnabled(ok);
    }

    if (actionCollection()->action(QStringLiteral("tools_toggle_comment"))) {
        actionCollection()->action(QStringLiteral("tools_toggle_comment"))->setEnabled(ok);
    }

    // show folding bar if "view defaults" says so, otherwise enable/disable only the menu entry
    updateFoldingConfig();
}

int KTextEditor::ViewPrivate::virtualCursorColumn() const
{
    return doc()->toVirtualColumn(m_viewInternal->cursorPosition());
}

void KTextEditor::ViewPrivate::notifyMousePositionChanged(const KTextEditor::Cursor &newPosition)
{
    emit mousePositionChanged(this, newPosition);
}

// BEGIN KTextEditor::SelectionInterface stuff

bool KTextEditor::ViewPrivate::setSelection(const KTextEditor::Range &selection)
{
    /**
     * anything to do?
     */
    if (selection == m_selection) {
        return true;
    }

    /**
     * backup old range
     */
    KTextEditor::Range oldSelection = m_selection;

    /**
     * set new range
     */
    m_selection.setRange(selection.isEmpty() ? KTextEditor::Range::invalid() : selection);

    /**
     * trigger update of correct area
     */
    tagSelection(oldSelection);
    repaintText(true);

    /**
     * emit holy signal
     */
    emit selectionChanged(this);

    /**
     * be done
     */
    return true;
}

bool KTextEditor::ViewPrivate::clearSelection()
{
    return clearSelection(true);
}

bool KTextEditor::ViewPrivate::clearSelection(bool redraw, bool finishedChangingSelection)
{
    /**
     * no selection, nothing to do...
     */
    if (!selection()) {
        return false;
    }

    /**
     * backup old range
     */
    KTextEditor::Range oldSelection = m_selection;

    /**
     * invalidate current selection
     */
    m_selection.setRange(KTextEditor::Range::invalid());

    /**
     * trigger update of correct area
     */
    tagSelection(oldSelection);
    if (redraw) {
        repaintText(true);
    }

    /**
     * emit holy signal
     */
    if (finishedChangingSelection) {
        emit selectionChanged(this);
    }

    /**
     * be done
     */
    return true;
}

bool KTextEditor::ViewPrivate::selection() const
{
    if (!wrapCursor()) {
        return m_selection != KTextEditor::Range::invalid();
    } else {
        return m_selection.toRange().isValid();
    }
}

QString KTextEditor::ViewPrivate::selectionText() const
{
    return doc()->text(m_selection, blockSelect);
}

bool KTextEditor::ViewPrivate::removeSelectedText()
{
    if (!selection()) {
        return false;
    }

    doc()->editStart();

    // Optimization: clear selection before removing text
    KTextEditor::Range selection = m_selection;

    doc()->removeText(selection, blockSelect);

    // don't redraw the cleared selection - that's done in editEnd().
    if (blockSelect) {
        int selectionColumn = qMin(doc()->toVirtualColumn(selection.start()), doc()->toVirtualColumn(selection.end()));
        KTextEditor::Range newSelection = selection;
        newSelection.setStart(KTextEditor::Cursor(newSelection.start().line(), doc()->fromVirtualColumn(newSelection.start().line(), selectionColumn)));
        newSelection.setEnd(KTextEditor::Cursor(newSelection.end().line(), doc()->fromVirtualColumn(newSelection.end().line(), selectionColumn)));
        setSelection(newSelection);
        setCursorPositionInternal(newSelection.start());
    } else {
        clearSelection(false);
    }

    doc()->editEnd();

    return true;
}

bool KTextEditor::ViewPrivate::selectAll()
{
    setBlockSelection(false);
    top();
    shiftBottom();
    return true;
}

bool KTextEditor::ViewPrivate::cursorSelected(const KTextEditor::Cursor &cursor)
{
    KTextEditor::Cursor ret = cursor;
    if ((!blockSelect) && (ret.column() < 0)) {
        ret.setColumn(0);
    }

    if (blockSelect)
        return cursor.line() >= m_selection.start().line() && ret.line() <= m_selection.end().line() && ret.column() >= m_selection.start().column() && ret.column() <= m_selection.end().column();
    else {
        return m_selection.toRange().contains(cursor) || m_selection.end() == cursor;
    }
}

bool KTextEditor::ViewPrivate::lineSelected(int line)
{
    return !blockSelect && m_selection.toRange().containsLine(line);
}

bool KTextEditor::ViewPrivate::lineEndSelected(const KTextEditor::Cursor &lineEndPos)
{
    return (!blockSelect) && (lineEndPos.line() > m_selection.start().line() || (lineEndPos.line() == m_selection.start().line() && (m_selection.start().column() < lineEndPos.column() || lineEndPos.column() == -1))) &&
        (lineEndPos.line() < m_selection.end().line() || (lineEndPos.line() == m_selection.end().line() && (lineEndPos.column() <= m_selection.end().column() && lineEndPos.column() != -1)));
}

bool KTextEditor::ViewPrivate::lineHasSelected(int line)
{
    return selection() && m_selection.toRange().containsLine(line);
}

bool KTextEditor::ViewPrivate::lineIsSelection(int line)
{
    return (line == m_selection.start().line() && line == m_selection.end().line());
}

void KTextEditor::ViewPrivate::tagSelection(const KTextEditor::Range &oldSelection)
{
    if (selection()) {
        if (oldSelection.start().line() == -1) {
            // We have to tag the whole lot if
            // 1) we have a selection, and:
            //  a) it's new; or
            tagLines(m_selection, true);

        } else if (blockSelection() && (oldSelection.start().column() != m_selection.start().column() || oldSelection.end().column() != m_selection.end().column())) {
            //  b) we're in block selection mode and the columns have changed
            tagLines(m_selection, true);
            tagLines(oldSelection, true);

        } else {
            if (oldSelection.start() != m_selection.start()) {
                if (oldSelection.start() < m_selection.start()) {
                    tagLines(oldSelection.start(), m_selection.start(), true);
                } else {
                    tagLines(m_selection.start(), oldSelection.start(), true);
                }
            }

            if (oldSelection.end() != m_selection.end()) {
                if (oldSelection.end() < m_selection.end()) {
                    tagLines(oldSelection.end(), m_selection.end(), true);
                } else {
                    tagLines(m_selection.end(), oldSelection.end(), true);
                }
            }
        }

    } else {
        // No more selection, clean up
        tagLines(oldSelection, true);
    }
}

void KTextEditor::ViewPrivate::selectWord(const KTextEditor::Cursor &cursor)
{
    setSelection(doc()->wordRangeAt(cursor));
}

void KTextEditor::ViewPrivate::selectLine(const KTextEditor::Cursor &cursor)
{
    int line = cursor.line();
    if (line + 1 >= doc()->lines()) {
        setSelection(KTextEditor::Range(line, 0, line, doc()->lineLength(line)));
    } else {
        setSelection(KTextEditor::Range(line, 0, line + 1, 0));
    }
}

void KTextEditor::ViewPrivate::cut()
{
    if (!selection() && !m_config->smartCopyCut()) {
        return;
    }

    copy();
    if (!selection()) {
        selectLine(cursorPosition());
    }
    removeSelectedText();
}

void KTextEditor::ViewPrivate::copy() const
{
    QString text;

    if (!selection()) {
        if (!m_config->smartCopyCut()) {
            return;
        }
        text = doc()->line(cursorPosition().line()) + QLatin1Char('\n');
        m_viewInternal->moveEdge(KateViewInternal::left, false);
    } else {
        text = selectionText();
    }

    // copy to clipboard and our history!
    KTextEditor::EditorPrivate::self()->copyToClipboard(text);
}

void KTextEditor::ViewPrivate::applyWordWrap()
{
    int first = selectionRange().start().line();
    int last = selectionRange().end().line();

    if (first == last) {
        // Either no selection or only one line selected, wrap only the current line
        first = cursorPosition().line();
        last = first;
    }

    doc()->wrapParagraph(first, last);
}

// END

// BEGIN KTextEditor::BlockSelectionInterface stuff

bool KTextEditor::ViewPrivate::blockSelection() const
{
    return blockSelect;
}

bool KTextEditor::ViewPrivate::setBlockSelection(bool on)
{
    if (on != blockSelect) {
        blockSelect = on;

        KTextEditor::Range oldSelection = m_selection;

        const bool hadSelection = clearSelection(false, false);

        setSelection(oldSelection);

        m_toggleBlockSelection->setChecked(blockSelection());

        // when leaving block selection mode, if cursor is at an invalid position or past the end of the
        // line, move the cursor to the last column of the current line unless cursor wrapping is off
        ensureCursorColumnValid();

        if (!hadSelection) {
            // emit selectionChanged() according to the KTextEditor::View api
            // documentation also if there is no selection around. This is needed,
            // as e.g. the Kate App status bar uses this signal to update the state
            // of the selection mode (block selection, line based selection)
            emit selectionChanged(this);
        }
    }

    return true;
}

bool KTextEditor::ViewPrivate::toggleBlockSelection()
{
    m_toggleBlockSelection->setChecked(!blockSelect);
    return setBlockSelection(!blockSelect);
}

bool KTextEditor::ViewPrivate::wrapCursor() const
{
    return !blockSelection();
}

// END

void KTextEditor::ViewPrivate::slotTextInserted(KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &text)
{
    emit textInserted(view, position, text);
}

bool KTextEditor::ViewPrivate::insertTemplateInternal(const KTextEditor::Cursor &c, const QString &templateString, const QString &script)
{
    /**
     * no empty templates
     */
    if (templateString.isEmpty()) {
        return false;
    }

    /**
     * not for read-only docs
     */
    if (!doc()->isReadWrite()) {
        return false;
    }

    /**
     * only one handler maybe active at a time; store it in the document.
     * Clear it first to make sure at no time two handlers are active at once
     */
    doc()->setActiveTemplateHandler(nullptr);
    doc()->setActiveTemplateHandler(new KateTemplateHandler(this, c, templateString, script, doc()->undoManager()));
    return true;
}

bool KTextEditor::ViewPrivate::tagLines(KTextEditor::Range range, bool realRange)
{
    return tagLines(range.start(), range.end(), realRange);
}

void KTextEditor::ViewPrivate::deactivateEditActions()
{
    for (QAction *action : qAsConst(m_editActions)) {
        action->setEnabled(false);
    }
}

void KTextEditor::ViewPrivate::activateEditActions()
{
    for (QAction *action : qAsConst(m_editActions)) {
        action->setEnabled(true);
    }
}

bool KTextEditor::ViewPrivate::mouseTrackingEnabled() const
{
    // FIXME support
    return true;
}

bool KTextEditor::ViewPrivate::setMouseTrackingEnabled(bool)
{
    // FIXME support
    return true;
}

bool KTextEditor::ViewPrivate::isCompletionActive() const
{
    return completionWidget()->isCompletionActive();
}

KateCompletionWidget *KTextEditor::ViewPrivate::completionWidget() const
{
    if (!m_completionWidget) {
        m_completionWidget = new KateCompletionWidget(const_cast<KTextEditor::ViewPrivate *>(this));
    }

    return m_completionWidget;
}

void KTextEditor::ViewPrivate::startCompletion(const KTextEditor::Range &word, KTextEditor::CodeCompletionModel *model)
{
    completionWidget()->startCompletion(word, model);
}

void KTextEditor::ViewPrivate::abortCompletion()
{
    completionWidget()->abortCompletion();
}

void KTextEditor::ViewPrivate::forceCompletion()
{
    completionWidget()->execute();
}

void KTextEditor::ViewPrivate::registerCompletionModel(KTextEditor::CodeCompletionModel *model)
{
    completionWidget()->registerCompletionModel(model);
}

void KTextEditor::ViewPrivate::unregisterCompletionModel(KTextEditor::CodeCompletionModel *model)
{
    completionWidget()->unregisterCompletionModel(model);
}

bool KTextEditor::ViewPrivate::isCompletionModelRegistered(KTextEditor::CodeCompletionModel *model) const
{
    return completionWidget()->isCompletionModelRegistered(model);
}

bool KTextEditor::ViewPrivate::isAutomaticInvocationEnabled() const
{
    return !m_temporaryAutomaticInvocationDisabled && m_config->automaticCompletionInvocation();
}

void KTextEditor::ViewPrivate::setAutomaticInvocationEnabled(bool enabled)
{
    config()->setValue(KateViewConfig::AutomaticCompletionInvocation, enabled);
}

void KTextEditor::ViewPrivate::sendCompletionExecuted(const KTextEditor::Cursor &position, KTextEditor::CodeCompletionModel *model, const QModelIndex &index)
{
    emit completionExecuted(this, position, model, index);
}

void KTextEditor::ViewPrivate::sendCompletionAborted()
{
    emit completionAborted(this);
}

void KTextEditor::ViewPrivate::paste(const QString *textToPaste)
{
    m_temporaryAutomaticInvocationDisabled = true;
    doc()->paste(this, textToPaste ? *textToPaste : QApplication::clipboard()->text(QClipboard::Clipboard));
    m_temporaryAutomaticInvocationDisabled = false;
}

bool KTextEditor::ViewPrivate::setCursorPosition(KTextEditor::Cursor position)
{
    return setCursorPositionInternal(position, 1, true);
}

KTextEditor::Cursor KTextEditor::ViewPrivate::cursorPosition() const
{
    return m_viewInternal->cursorPosition();
}

KTextEditor::Cursor KTextEditor::ViewPrivate::cursorPositionVirtual() const
{
    return KTextEditor::Cursor(m_viewInternal->cursorPosition().line(), virtualCursorColumn());
}

QPoint KTextEditor::ViewPrivate::cursorToCoordinate(const KTextEditor::Cursor &cursor) const
{
    // map from ViewInternal to View coordinates
    const QPoint pt = m_viewInternal->cursorToCoordinate(cursor, true, false);
    return pt == QPoint(-1, -1) ? pt : m_viewInternal->mapToParent(pt);
}

KTextEditor::Cursor KTextEditor::ViewPrivate::coordinatesToCursor(const QPoint &coords) const
{
    // map from View to ViewInternal coordinates
    return m_viewInternal->coordinatesToCursor(m_viewInternal->mapFromParent(coords), false);
}

QPoint KTextEditor::ViewPrivate::cursorPositionCoordinates() const
{
    // map from ViewInternal to View coordinates
    const QPoint pt = m_viewInternal->cursorCoordinates(false);
    return pt == QPoint(-1, -1) ? pt : m_viewInternal->mapToParent(pt);
}

void KTextEditor::ViewPrivate::setScrollPositionInternal(KTextEditor::Cursor &cursor)
{
    m_viewInternal->scrollPos(cursor, false, true, false);
}

void KTextEditor::ViewPrivate::setHorizontalScrollPositionInternal(int x)
{
    m_viewInternal->scrollColumns(x);
}

KTextEditor::Cursor KTextEditor::ViewPrivate::maxScrollPositionInternal() const
{
    return m_viewInternal->maxStartPos(true);
}

int KTextEditor::ViewPrivate::firstDisplayedLineInternal(LineType lineType) const
{
    if (lineType == RealLine) {
        return m_textFolding.visibleLineToLine(m_viewInternal->startLine());
    } else {
        return m_viewInternal->startLine();
    }
}

int KTextEditor::ViewPrivate::lastDisplayedLineInternal(LineType lineType) const
{
    if (lineType == RealLine) {
        return m_textFolding.visibleLineToLine(m_viewInternal->endLine());
    } else {
        return m_viewInternal->endLine();
    }
}

QRect KTextEditor::ViewPrivate::textAreaRectInternal() const
{
    const auto sourceRect = m_viewInternal->rect();
    const auto topLeft = m_viewInternal->mapTo(this, sourceRect.topLeft());
    const auto bottomRight = m_viewInternal->mapTo(this, sourceRect.bottomRight());
    return {topLeft, bottomRight};
}

bool KTextEditor::ViewPrivate::setCursorPositionVisual(const KTextEditor::Cursor &position)
{
    return setCursorPositionInternal(position, doc()->config()->tabWidth(), true);
}

QString KTextEditor::ViewPrivate::currentTextLine()
{
    return doc()->line(cursorPosition().line());
}

QTextLayout *KTextEditor::ViewPrivate::textLayout(int line) const
{
    KateLineLayoutPtr thisLine = m_viewInternal->cache()->line(line);

    return thisLine->isValid() ? thisLine->layout() : nullptr;
}

QTextLayout *KTextEditor::ViewPrivate::textLayout(const KTextEditor::Cursor &pos) const
{
    KateLineLayoutPtr thisLine = m_viewInternal->cache()->line(pos);

    return thisLine->isValid() ? thisLine->layout() : nullptr;
}

void KTextEditor::ViewPrivate::indent()
{
    KTextEditor::Cursor c(cursorPosition().line(), 0);
    KTextEditor::Range r = selection() ? selectionRange() : KTextEditor::Range(c, c);
    doc()->indent(r, 1);
}

void KTextEditor::ViewPrivate::unIndent()
{
    KTextEditor::Cursor c(cursorPosition().line(), 0);
    KTextEditor::Range r = selection() ? selectionRange() : KTextEditor::Range(c, c);
    doc()->indent(r, -1);
}

void KTextEditor::ViewPrivate::cleanIndent()
{
    KTextEditor::Cursor c(cursorPosition().line(), 0);
    KTextEditor::Range r = selection() ? selectionRange() : KTextEditor::Range(c, c);
    doc()->indent(r, 0);
}

void KTextEditor::ViewPrivate::align()
{
    // no selection: align current line; selection: use selection range
    const int line = cursorPosition().line();
    KTextEditor::Range alignRange(KTextEditor::Cursor(line, 0), KTextEditor::Cursor(line, 0));
    if (selection()) {
        alignRange = selectionRange();
    }

    doc()->align(this, alignRange);
}

void KTextEditor::ViewPrivate::comment()
{
    m_selection.setInsertBehaviors(Kate::TextRange::ExpandLeft | Kate::TextRange::ExpandRight);
    doc()->comment(this, cursorPosition().line(), cursorPosition().column(), 1);
    m_selection.setInsertBehaviors(Kate::TextRange::ExpandRight);
}

void KTextEditor::ViewPrivate::uncomment()
{
    doc()->comment(this, cursorPosition().line(), cursorPosition().column(), -1);
}

void KTextEditor::ViewPrivate::toggleComment()
{
    m_selection.setInsertBehaviors(Kate::TextRange::ExpandLeft | Kate::TextRange::ExpandRight);
    doc()->comment(this, cursorPosition().line(), cursorPosition().column(), 0);
    m_selection.setInsertBehaviors(Kate::TextRange::ExpandRight);
}

void KTextEditor::ViewPrivate::uppercase()
{
    doc()->transform(this, cursorPosition(), KTextEditor::DocumentPrivate::Uppercase);
}

void KTextEditor::ViewPrivate::killLine()
{
    if (m_selection.isEmpty()) {
        doc()->removeLine(cursorPosition().line());
    } else {
        doc()->editStart();
        // cache endline, else that moves and we might delete complete document if last line is selected!
        for (int line = m_selection.end().line(), endLine = m_selection.start().line(); line >= endLine; line--) {
            doc()->removeLine(line);
        }
        doc()->editEnd();
    }
}

void KTextEditor::ViewPrivate::lowercase()
{
    doc()->transform(this, cursorPosition(), KTextEditor::DocumentPrivate::Lowercase);
}

void KTextEditor::ViewPrivate::capitalize()
{
    doc()->editStart();
    doc()->transform(this, cursorPosition(), KTextEditor::DocumentPrivate::Lowercase);
    doc()->transform(this, cursorPosition(), KTextEditor::DocumentPrivate::Capitalize);
    doc()->editEnd();
}

void KTextEditor::ViewPrivate::keyReturn()
{
    doc()->newLine(this);
    m_viewInternal->iconBorder()->updateForCursorLineChange();
    m_viewInternal->updateView();
}

void KTextEditor::ViewPrivate::smartNewline()
{
    const KTextEditor::Cursor cursor = cursorPosition();
    const int ln = cursor.line();
    Kate::TextLine line = doc()->kateTextLine(ln);
    int col = qMin(cursor.column(), line->firstChar());
    if (col != -1) {
        while (line->length() > col && !(line->at(col).isLetterOrNumber() || line->at(col) == QLatin1Char('_')) && col < cursor.column()) {
            ++col;
        }
    } else {
        col = line->length(); // stay indented
    }
    doc()->editStart();
    doc()->editWrapLine(ln, cursor.column());
    doc()->insertText(KTextEditor::Cursor(ln + 1, 0), line->string(0, col));
    doc()->editEnd();

    m_viewInternal->updateView();
}

void KTextEditor::ViewPrivate::noIndentNewline()
{
    doc()->newLine(this, KTextEditor::DocumentPrivate::NoIndent);
    m_viewInternal->iconBorder()->updateForCursorLineChange();
    m_viewInternal->updateView();
}

void KTextEditor::ViewPrivate::backspace()
{
    doc()->backspace(this, cursorPosition());
}

void KTextEditor::ViewPrivate::insertTab()
{
    doc()->insertTab(this, cursorPosition());
}

void KTextEditor::ViewPrivate::deleteWordLeft()
{
    doc()->editStart();
    m_viewInternal->wordPrev(true);
    KTextEditor::Range selection = selectionRange();
    removeSelectedText();
    doc()->editEnd();
    m_viewInternal->tagRange(selection, true);
    m_viewInternal->updateDirty();
}

void KTextEditor::ViewPrivate::keyDelete()
{
    doc()->del(this, cursorPosition());
}

void KTextEditor::ViewPrivate::deleteWordRight()
{
    doc()->editStart();
    m_viewInternal->wordNext(true);
    KTextEditor::Range selection = selectionRange();
    removeSelectedText();
    doc()->editEnd();
    m_viewInternal->tagRange(selection, true);
    m_viewInternal->updateDirty();
}

void KTextEditor::ViewPrivate::transpose()
{
    doc()->transpose(cursorPosition());
}

void KTextEditor::ViewPrivate::cursorLeft()
{
    if (selection() && !config()->persistentSelection()) {
        if (currentTextLine().isRightToLeft()) {
            m_viewInternal->updateCursor(selectionRange().end());
            setSelection(KTextEditor::Range::invalid());
        } else {
            m_viewInternal->updateCursor(selectionRange().start());
            setSelection(KTextEditor::Range::invalid());
        }

    } else {
        if (currentTextLine().isRightToLeft()) {
            m_viewInternal->cursorNextChar();
        } else {
            m_viewInternal->cursorPrevChar();
        }
    }
}

void KTextEditor::ViewPrivate::shiftCursorLeft()
{
    if (currentTextLine().isRightToLeft()) {
        m_viewInternal->cursorNextChar(true);
    } else {
        m_viewInternal->cursorPrevChar(true);
    }
}

void KTextEditor::ViewPrivate::cursorRight()
{
    if (selection() && !config()->persistentSelection()) {
        if (currentTextLine().isRightToLeft()) {
            m_viewInternal->updateCursor(selectionRange().start());
            setSelection(KTextEditor::Range::invalid());
        } else {
            m_viewInternal->updateCursor(selectionRange().end());
            setSelection(KTextEditor::Range::invalid());
        }

    } else {
        if (currentTextLine().isRightToLeft()) {
            m_viewInternal->cursorPrevChar();
        } else {
            m_viewInternal->cursorNextChar();
        }
    }
}

void KTextEditor::ViewPrivate::shiftCursorRight()
{
    if (currentTextLine().isRightToLeft()) {
        m_viewInternal->cursorPrevChar(true);
    } else {
        m_viewInternal->cursorNextChar(true);
    }
}

void KTextEditor::ViewPrivate::wordLeft()
{
    if (currentTextLine().isRightToLeft()) {
        m_viewInternal->wordNext();
    } else {
        m_viewInternal->wordPrev();
    }
}

void KTextEditor::ViewPrivate::shiftWordLeft()
{
    if (currentTextLine().isRightToLeft()) {
        m_viewInternal->wordNext(true);
    } else {
        m_viewInternal->wordPrev(true);
    }
}

void KTextEditor::ViewPrivate::wordRight()
{
    if (currentTextLine().isRightToLeft()) {
        m_viewInternal->wordPrev();
    } else {
        m_viewInternal->wordNext();
    }
}

void KTextEditor::ViewPrivate::shiftWordRight()
{
    if (currentTextLine().isRightToLeft()) {
        m_viewInternal->wordPrev(true);
    } else {
        m_viewInternal->wordNext(true);
    }
}

void KTextEditor::ViewPrivate::home()
{
    m_viewInternal->home();
}

void KTextEditor::ViewPrivate::shiftHome()
{
    m_viewInternal->home(true);
}

void KTextEditor::ViewPrivate::end()
{
    m_viewInternal->end();
}

void KTextEditor::ViewPrivate::shiftEnd()
{
    m_viewInternal->end(true);
}

void KTextEditor::ViewPrivate::up()
{
    m_viewInternal->cursorUp();
}

void KTextEditor::ViewPrivate::shiftUp()
{
    m_viewInternal->cursorUp(true);
}

void KTextEditor::ViewPrivate::down()
{
    m_viewInternal->cursorDown();
}

void KTextEditor::ViewPrivate::shiftDown()
{
    m_viewInternal->cursorDown(true);
}

void KTextEditor::ViewPrivate::scrollUp()
{
    m_viewInternal->scrollUp();
}

void KTextEditor::ViewPrivate::scrollDown()
{
    m_viewInternal->scrollDown();
}

void KTextEditor::ViewPrivate::topOfView()
{
    m_viewInternal->topOfView();
}

void KTextEditor::ViewPrivate::shiftTopOfView()
{
    m_viewInternal->topOfView(true);
}

void KTextEditor::ViewPrivate::bottomOfView()
{
    m_viewInternal->bottomOfView();
}

void KTextEditor::ViewPrivate::shiftBottomOfView()
{
    m_viewInternal->bottomOfView(true);
}

void KTextEditor::ViewPrivate::pageUp()
{
    m_viewInternal->pageUp();
}

void KTextEditor::ViewPrivate::shiftPageUp()
{
    m_viewInternal->pageUp(true);
}

void KTextEditor::ViewPrivate::pageDown()
{
    m_viewInternal->pageDown();
}

void KTextEditor::ViewPrivate::shiftPageDown()
{
    m_viewInternal->pageDown(true);
}

void KTextEditor::ViewPrivate::top()
{
    m_viewInternal->top_home();
}

void KTextEditor::ViewPrivate::shiftTop()
{
    m_viewInternal->top_home(true);
}

void KTextEditor::ViewPrivate::bottom()
{
    m_viewInternal->bottom_end();
}

void KTextEditor::ViewPrivate::shiftBottom()
{
    m_viewInternal->bottom_end(true);
}

void KTextEditor::ViewPrivate::toMatchingBracket()
{
    m_viewInternal->cursorToMatchingBracket();
}

void KTextEditor::ViewPrivate::shiftToMatchingBracket()
{
    m_viewInternal->cursorToMatchingBracket(true);
}

void KTextEditor::ViewPrivate::toPrevModifiedLine()
{
    const int startLine = cursorPosition().line() - 1;
    const int line = doc()->findTouchedLine(startLine, false);
    if (line >= 0) {
        KTextEditor::Cursor c(line, 0);
        m_viewInternal->updateSelection(c, false);
        m_viewInternal->updateCursor(c);
    }
}

void KTextEditor::ViewPrivate::toNextModifiedLine()
{
    const int startLine = cursorPosition().line() + 1;
    const int line = doc()->findTouchedLine(startLine, true);
    if (line >= 0) {
        KTextEditor::Cursor c(line, 0);
        m_viewInternal->updateSelection(c, false);
        m_viewInternal->updateCursor(c);
    }
}

KTextEditor::Range KTextEditor::ViewPrivate::selectionRange() const
{
    return m_selection;
}

KTextEditor::Document *KTextEditor::ViewPrivate::document() const
{
    return m_doc;
}

void KTextEditor::ViewPrivate::setContextMenu(QMenu *menu)
{
    if (m_contextMenu) {
        disconnect(m_contextMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
        disconnect(m_contextMenu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
    }
    m_contextMenu = menu;
    m_userContextMenuSet = true;

    if (m_contextMenu) {
        connect(m_contextMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
        connect(m_contextMenu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
    }
}

QMenu *KTextEditor::ViewPrivate::contextMenu() const
{
    if (m_userContextMenuSet) {
        return m_contextMenu;
    } else {
        KXMLGUIClient *client = const_cast<KTextEditor::ViewPrivate *>(this);
        while (client->parentClient()) {
            client = client->parentClient();
        }

        // qCDebug(LOG_KTE) << "looking up all menu containers";
        if (client->factory()) {
            const QList<QWidget *> menuContainers = client->factory()->containers(QStringLiteral("menu"));
            for (QWidget *w : menuContainers) {
                if (w->objectName() == QLatin1String("ktexteditor_popup")) {
                    // perhaps optimize this block
                    QMenu *menu = (QMenu *)w;
                    // menu is a reusable instance shared among all views. Therefore,
                    // disconnect the current receiver(s) from the menu show/hide signals
                    // before connecting `this` view. This ensures that only the current
                    // view gets a signal when the menu is about to be shown or hidden,
                    // and not also the view(s) that previously had the menu open.
                    disconnect(menu, SIGNAL(aboutToShow()), nullptr, nullptr);
                    disconnect(menu, SIGNAL(aboutToHide()), nullptr, nullptr);
                    connect(menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
                    connect(menu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
                    return menu;
                }
            }
        }
    }
    return nullptr;
}

QMenu *KTextEditor::ViewPrivate::defaultContextMenu(QMenu *menu) const
{
    if (!menu) {
        menu = new QMenu(const_cast<KTextEditor::ViewPrivate *>(this));
    }

    menu->addAction(m_editUndo);
    menu->addAction(m_editRedo);
    menu->addSeparator();
    menu->addAction(m_cut);
    menu->addAction(m_copy);
    menu->addAction(m_paste);
    menu->addSeparator();
    menu->addAction(m_selectAll);
    menu->addAction(m_deSelect);
    if (QAction *spellingSuggestions = actionCollection()->action(QStringLiteral("spelling_suggestions"))) {
        menu->addSeparator();
        menu->addAction(spellingSuggestions);
    }
    if (QAction *bookmark = actionCollection()->action(QStringLiteral("bookmarks"))) {
        menu->addSeparator();
        menu->addAction(bookmark);
    }
    return menu;
}

void KTextEditor::ViewPrivate::aboutToShowContextMenu()
{
    QMenu *menu = qobject_cast<QMenu *>(sender());

    if (menu) {
        emit contextMenuAboutToShow(this, menu);
    }
}

void KTextEditor::ViewPrivate::aboutToHideContextMenu()
{
    m_spellingMenu->setUseMouseForMisspelledRange(false);
}

// BEGIN ConfigInterface stff
QStringList KTextEditor::ViewPrivate::configKeys() const
{
    static const QStringList keys = {QStringLiteral("icon-bar"),
                                     QStringLiteral("line-numbers"),
                                     QStringLiteral("dynamic-word-wrap"),
                                     QStringLiteral("background-color"),
                                     QStringLiteral("selection-color"),
                                     QStringLiteral("search-highlight-color"),
                                     QStringLiteral("replace-highlight-color"),
                                     QStringLiteral("default-mark-type"),
                                     QStringLiteral("allow-mark-menu"),
                                     QStringLiteral("folding-bar"),
                                     QStringLiteral("folding-preview"),
                                     QStringLiteral("icon-border-color"),
                                     QStringLiteral("folding-marker-color"),
                                     QStringLiteral("line-number-color"),
                                     QStringLiteral("current-line-number-color"),
                                     QStringLiteral("modification-markers"),
                                     QStringLiteral("keyword-completion"),
                                     QStringLiteral("word-count"),
                                     QStringLiteral("line-count"),
                                     QStringLiteral("scrollbar-minimap"),
                                     QStringLiteral("scrollbar-preview"),
                                     QStringLiteral("font")};
    return keys;
}

QVariant KTextEditor::ViewPrivate::configValue(const QString &key)
{
    if (key == QLatin1String("icon-bar")) {
        return config()->iconBar();
    } else if (key == QLatin1String("line-numbers")) {
        return config()->lineNumbers();
    } else if (key == QLatin1String("dynamic-word-wrap")) {
        return config()->dynWordWrap();
    } else if (key == QLatin1String("background-color")) {
        return renderer()->config()->backgroundColor();
    } else if (key == QLatin1String("selection-color")) {
        return renderer()->config()->selectionColor();
    } else if (key == QLatin1String("search-highlight-color")) {
        return renderer()->config()->searchHighlightColor();
    } else if (key == QLatin1String("replace-highlight-color")) {
        return renderer()->config()->replaceHighlightColor();
    } else if (key == QLatin1String("default-mark-type")) {
        return config()->defaultMarkType();
    } else if (key == QLatin1String("allow-mark-menu")) {
        return config()->allowMarkMenu();
    } else if (key == QLatin1String("folding-bar")) {
        return config()->foldingBar();
    } else if (key == QLatin1String("folding-preview")) {
        return config()->foldingPreview();
    } else if (key == QLatin1String("icon-border-color")) {
        return renderer()->config()->iconBarColor();
    } else if (key == QLatin1String("folding-marker-color")) {
        return renderer()->config()->foldingColor();
    } else if (key == QLatin1String("line-number-color")) {
        return renderer()->config()->lineNumberColor();
    } else if (key == QLatin1String("current-line-number-color")) {
        return renderer()->config()->currentLineNumberColor();
    } else if (key == QLatin1String("modification-markers")) {
        return config()->lineModification();
    } else if (key == QLatin1String("keyword-completion")) {
        return config()->keywordCompletion();
    } else if (key == QLatin1String("word-count")) {
        return config()->showWordCount();
    } else if (key == QLatin1String("line-count")) {
        return config()->showLineCount();
    } else if (key == QLatin1String("scrollbar-minimap")) {
        return config()->scrollBarMiniMap();
    } else if (key == QLatin1String("scrollbar-preview")) {
        return config()->scrollBarPreview();
    } else if (key == QLatin1String("font")) {
        return renderer()->config()->baseFont();
    }

    // return invalid variant
    return QVariant();
}

void KTextEditor::ViewPrivate::setConfigValue(const QString &key, const QVariant &value)
{
    // First, try the new config interface
    if (config()->setValue(key, value)) {
        return;

    } else if (renderer()->config()->setValue(key, value)) {
        return;
    }

    // No success? Go the old way
    if (value.canConvert(QVariant::Color)) {
        if (key == QLatin1String("background-color")) {
            renderer()->config()->setBackgroundColor(value.value<QColor>());
        } else if (key == QLatin1String("selection-color")) {
            renderer()->config()->setSelectionColor(value.value<QColor>());
        } else if (key == QLatin1String("search-highlight-color")) {
            renderer()->config()->setSearchHighlightColor(value.value<QColor>());
        } else if (key == QLatin1String("replace-highlight-color")) {
            renderer()->config()->setReplaceHighlightColor(value.value<QColor>());
        } else if (key == QLatin1String("icon-border-color")) {
            renderer()->config()->setIconBarColor(value.value<QColor>());
        } else if (key == QLatin1String("folding-marker-color")) {
            renderer()->config()->setFoldingColor(value.value<QColor>());
        } else if (key == QLatin1String("line-number-color")) {
            renderer()->config()->setLineNumberColor(value.value<QColor>());
        } else if (key == QLatin1String("current-line-number-color")) {
            renderer()->config()->setCurrentLineNumberColor(value.value<QColor>());
        }

    } else if (value.type() == QVariant::Bool) {
        // Note explicit type check above. If we used canConvert, then
        // values of type UInt will be trapped here.
        if (key == QLatin1String("dynamic-word-wrap")) {
            config()->setDynWordWrap(value.toBool());
        } else if (key == QLatin1String("word-count")) {
            config()->setShowWordCount(value.toBool());
        } else if (key == QLatin1String("line-count")) {
            config()->setShowLineCount(value.toBool());
        }

    } else if (value.canConvert(QVariant::Font)) {
        if (key == QLatin1String("font")) {
            renderer()->config()->setFont(value.value<QFont>());
        }
    }
}

// END ConfigInterface

void KTextEditor::ViewPrivate::userInvokedCompletion()
{
    completionWidget()->userInvokedCompletion();
}

KateViewBar *KTextEditor::ViewPrivate::bottomViewBar() const
{
    return m_bottomViewBar;
}

KateGotoBar *KTextEditor::ViewPrivate::gotoBar()
{
    if (!m_gotoBar) {
        m_gotoBar = new KateGotoBar(this);
        bottomViewBar()->addBarWidget(m_gotoBar);
    }

    return m_gotoBar;
}

KateDictionaryBar *KTextEditor::ViewPrivate::dictionaryBar()
{
    if (!m_dictionaryBar) {
        m_dictionaryBar = new KateDictionaryBar(this);
        bottomViewBar()->addBarWidget(m_dictionaryBar);
    }

    return m_dictionaryBar;
}

void KTextEditor::ViewPrivate::setAnnotationModel(KTextEditor::AnnotationModel *model)
{
    KTextEditor::AnnotationModel *oldmodel = m_annotationModel;
    m_annotationModel = model;
    m_viewInternal->m_leftBorder->annotationModelChanged(oldmodel, m_annotationModel);
}

KTextEditor::AnnotationModel *KTextEditor::ViewPrivate::annotationModel() const
{
    return m_annotationModel;
}

void KTextEditor::ViewPrivate::setAnnotationBorderVisible(bool visible)
{
    m_viewInternal->m_leftBorder->setAnnotationBorderOn(visible);
}

bool KTextEditor::ViewPrivate::isAnnotationBorderVisible() const
{
    return m_viewInternal->m_leftBorder->annotationBorderOn();
}

KTextEditor::AbstractAnnotationItemDelegate *KTextEditor::ViewPrivate::annotationItemDelegate() const
{
    return m_viewInternal->m_leftBorder->annotationItemDelegate();
}

void KTextEditor::ViewPrivate::setAnnotationItemDelegate(KTextEditor::AbstractAnnotationItemDelegate *delegate)
{
    m_viewInternal->m_leftBorder->setAnnotationItemDelegate(delegate);
}

bool KTextEditor::ViewPrivate::uniformAnnotationItemSizes() const
{
    return m_viewInternal->m_leftBorder->uniformAnnotationItemSizes();
}

void KTextEditor::ViewPrivate::setAnnotationUniformItemSizes(bool enable)
{
    m_viewInternal->m_leftBorder->setAnnotationUniformItemSizes(enable);
}

KTextEditor::Range KTextEditor::ViewPrivate::visibleRange()
{
    // ensure that the view is up-to-date, otherwise 'endPos()' might fail!
    if (!m_viewInternal->endPos().isValid()) {
        m_viewInternal->updateView();
    }
    return KTextEditor::Range(m_viewInternal->toRealCursor(m_viewInternal->startPos()), m_viewInternal->toRealCursor(m_viewInternal->endPos()));
}

bool KTextEditor::ViewPrivate::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::StyleChange:
        setupLayout();
        return true;
    default:
        return KTextEditor::View::event(e);
    }
}

void KTextEditor::ViewPrivate::paintEvent(QPaintEvent *e)
{
    // base class
    KTextEditor::View::paintEvent(e);

    const QRect contentsRect = m_topSpacer->geometry() | m_bottomSpacer->geometry() | m_leftSpacer->geometry() | m_rightSpacer->geometry();

    if (contentsRect.isValid()) {
        QStyleOptionFrame opt;
        opt.initFrom(this);
        opt.frameShape = QFrame::StyledPanel;
        opt.state |= QStyle::State_Sunken;

        // clear mouseOver and focus state
        // update from relevant widgets
        opt.state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
        const QList<QWidget *> widgets = QList<QWidget *>() << m_viewInternal << m_viewInternal->m_leftBorder << m_viewInternal->m_lineScroll << m_viewInternal->m_columnScroll;
        for (const QWidget *w : widgets) {
            if (w->hasFocus())
                opt.state |= QStyle::State_HasFocus;
            if (w->underMouse())
                opt.state |= QStyle::State_MouseOver;
        }

        // update rect
        opt.rect = contentsRect;

        // render
        QPainter paint(this);
        paint.setClipRegion(e->region());
        paint.setRenderHints(QPainter::Antialiasing);
        style()->drawControl(QStyle::CE_ShapedFrame, &opt, &paint, this);
    }
}

void KTextEditor::ViewPrivate::toggleOnTheFlySpellCheck(bool b)
{
    doc()->onTheFlySpellCheckingEnabled(b);
}

void KTextEditor::ViewPrivate::reflectOnTheFlySpellCheckStatus(bool enabled)
{
    m_spellingMenu->setVisible(enabled);
    m_toggleOnTheFlySpellCheck->setChecked(enabled);
}

KateSpellingMenu *KTextEditor::ViewPrivate::spellingMenu()
{
    return m_spellingMenu;
}

void KTextEditor::ViewPrivate::notifyAboutRangeChange(int startLine, int endLine, bool rangeWithAttribute)
{
#ifdef VIEW_RANGE_DEBUG
    // output args
    qCDebug(LOG_KTE) << "trigger attribute changed from" << startLine << "to" << endLine << "rangeWithAttribute" << rangeWithAttribute;
#endif

    // first call:
    if (!m_delayedUpdateTriggered) {
        m_delayedUpdateTriggered = true;
        m_lineToUpdateMin = -1;
        m_lineToUpdateMax = -1;

        // only set initial line range, if range with attribute!
        if (rangeWithAttribute) {
            m_lineToUpdateMin = startLine;
            m_lineToUpdateMax = endLine;
        }

        // emit queued signal and be done
        emit delayedUpdateOfView();
        return;
    }

    // ignore lines if no attribute
    if (!rangeWithAttribute) {
        return;
    }

    // update line range
    if (startLine != -1 && (m_lineToUpdateMin == -1 || startLine < m_lineToUpdateMin)) {
        m_lineToUpdateMin = startLine;
    }

    if (endLine != -1 && endLine > m_lineToUpdateMax) {
        m_lineToUpdateMax = endLine;
    }
}

void KTextEditor::ViewPrivate::slotDelayedUpdateOfView()
{
    if (!m_delayedUpdateTriggered) {
        return;
    }

#ifdef VIEW_RANGE_DEBUG
    // output args
    qCDebug(LOG_KTE) << "delayed attribute changed from" << m_lineToUpdateMin << "to" << m_lineToUpdateMax;
#endif

    // update ranges in
    updateRangesIn(KTextEditor::Attribute::ActivateMouseIn);
    updateRangesIn(KTextEditor::Attribute::ActivateCaretIn);

    // update view, if valid line range, else only feedback update wanted anyway
    if (m_lineToUpdateMin != -1 && m_lineToUpdateMax != -1) {
        tagLines(m_lineToUpdateMin, m_lineToUpdateMax, true);
        updateView(true);
    }

    // reset flags
    m_delayedUpdateTriggered = false;
    m_lineToUpdateMin = -1;
    m_lineToUpdateMax = -1;
}

void KTextEditor::ViewPrivate::updateRangesIn(KTextEditor::Attribute::ActivationType activationType)
{
    // new ranges with cursor in, default none
    QSet<Kate::TextRange *> newRangesIn;

    // on which range set we work?
    QSet<Kate::TextRange *> &oldSet = (activationType == KTextEditor::Attribute::ActivateMouseIn) ? m_rangesMouseIn : m_rangesCaretIn;

    // which cursor position to honor?
    KTextEditor::Cursor currentCursor = (activationType == KTextEditor::Attribute::ActivateMouseIn) ? m_viewInternal->mousePosition() : m_viewInternal->cursorPosition();

    // first: validate the remembered ranges
    QSet<Kate::TextRange *> validRanges;
    for (Kate::TextRange *range : qAsConst(oldSet)) {
        if (doc()->buffer().rangePointerValid(range)) {
            validRanges.insert(range);
        }
    }

    // cursor valid? else no new ranges can be found
    if (currentCursor.isValid() && currentCursor.line() < doc()->buffer().lines()) {
        // now: get current ranges for the line of cursor with an attribute
        const QList<Kate::TextRange *> rangesForCurrentCursor = doc()->buffer().rangesForLine(currentCursor.line(), this, false);

        // match which ranges really fit the given cursor
        for (Kate::TextRange *range : rangesForCurrentCursor) {
            // range has no dynamic attribute of right type and no feedback object
            if ((!range->attribute() || !range->attribute()->dynamicAttribute(activationType)) && !range->feedback()) {
                continue;
            }

            // range doesn't contain cursor, not interesting
            if ((range->start().insertBehavior() == KTextEditor::MovingCursor::StayOnInsert) ? (currentCursor < range->start().toCursor()) : (currentCursor <= range->start().toCursor())) {
                continue;
            }

            if ((range->end().insertBehavior() == KTextEditor::MovingCursor::StayOnInsert) ? (range->end().toCursor() <= currentCursor) : (range->end().toCursor() < currentCursor)) {
                continue;
            }

            // range contains cursor, was it already in old set?
            if (validRanges.contains(range)) {
                // insert in new, remove from old, be done with it
                newRangesIn.insert(range);
                validRanges.remove(range);
                continue;
            }

            // oh, new range, trigger update and insert into new set
            newRangesIn.insert(range);

            if (range->attribute() && range->attribute()->dynamicAttribute(activationType)) {
                notifyAboutRangeChange(range->start().line(), range->end().line(), true);
            }

            // feedback
            if (range->feedback()) {
                if (activationType == KTextEditor::Attribute::ActivateMouseIn) {
                    range->feedback()->mouseEnteredRange(range, this);
                } else {
                    range->feedback()->caretEnteredRange(range, this);
                    emit caretChangedRange(this);
                }
            }

#ifdef VIEW_RANGE_DEBUG
            // found new range for activation
            qCDebug(LOG_KTE) << "activated new range" << range << "by" << activationType;
#endif
        }
    }

    // now: notify for left ranges!
    for (Kate::TextRange *range : qAsConst(validRanges)) {
        // range valid + right dynamic attribute, trigger update
        if (range->toRange().isValid() && range->attribute() && range->attribute()->dynamicAttribute(activationType)) {
            notifyAboutRangeChange(range->start().line(), range->end().line(), true);
        }

        // feedback
        if (range->feedback()) {
            if (activationType == KTextEditor::Attribute::ActivateMouseIn) {
                range->feedback()->mouseExitedRange(range, this);
            } else {
                range->feedback()->caretExitedRange(range, this);
                emit caretChangedRange(this);
            }
        }
    }

    // set new ranges
    oldSet = newRangesIn;
}

void KTextEditor::ViewPrivate::postMessage(KTextEditor::Message *message, QList<QSharedPointer<QAction>> actions)
{
    // just forward to KateMessageWidget :-)
    auto messageWidget = m_messageWidgets[message->position()];
    if (!messageWidget) {
        // this branch is used for: TopInView, CenterInView, and BottomInView
        messageWidget = new KateMessageWidget(m_viewInternal, true);
        m_messageWidgets[message->position()] = messageWidget;
        m_notificationLayout->addWidget(messageWidget, message->position());
        connect(this, SIGNAL(displayRangeChanged(KTextEditor::ViewPrivate *)), messageWidget, SLOT(startAutoHideTimer()));
        connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View *, KTextEditor::Cursor)), messageWidget, SLOT(startAutoHideTimer()));
    }
    messageWidget->postMessage(message, std::move(actions));
}

KateMessageWidget *KTextEditor::ViewPrivate::messageWidget()
{
    return m_messageWidgets[KTextEditor::Message::TopInView];
}

void KTextEditor::ViewPrivate::saveFoldingState()
{
    m_savedFoldingState = m_textFolding.exportFoldingRanges();
}

void KTextEditor::ViewPrivate::applyFoldingState()
{
    m_textFolding.importFoldingRanges(m_savedFoldingState);
    m_savedFoldingState = QJsonDocument();
}

void KTextEditor::ViewPrivate::exportHtmlToFile(const QString &file)
{
    KateExporter(this).exportToFile(file);
}

void KTextEditor::ViewPrivate::exportHtmlToClipboard()
{
    KateExporter(this).exportToClipboard();
}

void KTextEditor::ViewPrivate::exportHtmlToFile()
{
    const QString file = QFileDialog::getSaveFileName(this, i18n("Export File as HTML"), doc()->documentName());
    if (!file.isEmpty()) {
        KateExporter(this).exportToFile(file);
    }
}

void KTextEditor::ViewPrivate::clearHighlights()
{
    qDeleteAll(m_rangesForHighlights);
    m_rangesForHighlights.clear();
    m_currentTextForHighlights.clear();
}

void KTextEditor::ViewPrivate::selectionChangedForHighlights()
{
    QString text;
    // if text of selection is still the same, abort
    if (selection() && selectionRange().onSingleLine()) {
        text = selectionText();
        if (text == m_currentTextForHighlights)
            return;
    }

    // text changed: remove all highlights + create new ones
    // (do not call clearHighlights(), since this also resets the m_currentTextForHighlights
    qDeleteAll(m_rangesForHighlights);
    m_rangesForHighlights.clear();

    // do not highlight strings with leading and trailing spaces
    if (!text.isEmpty() && (text.at(0).isSpace() || text.at(text.length() - 1).isSpace()))
        return;

    // trigger creation of ranges for current view range
    m_currentTextForHighlights = text;
    createHighlights();
}

void KTextEditor::ViewPrivate::createHighlights()
{
    // do nothing if no text to highlight
    if (m_currentTextForHighlights.isEmpty()) {
        return;
    }

    KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute());
    attr->setBackground(Qt::yellow);

    // set correct highlight color from Kate's color schema
    QColor fgColor = defaultStyleAttribute(KTextEditor::dsNormal)->foreground().color();
    QColor bgColor = renderer()->config()->searchHighlightColor();
    attr->setForeground(fgColor);
    attr->setBackground(bgColor);

    KTextEditor::Cursor start(visibleRange().start());
    KTextEditor::Range searchRange;

    /**
     * only add word boundary if we can find the text then
     * fixes $lala hl
     */
    QString regex = QRegExp::escape(m_currentTextForHighlights);
    if (QRegExp(QStringLiteral("\\b%1").arg(regex)).indexIn(QStringLiteral(" %1 ").arg(m_currentTextForHighlights)) != -1)
        regex = QStringLiteral("\\b%1").arg(regex);
    if (QRegExp(QStringLiteral("%1\\b").arg(regex)).indexIn(QStringLiteral(" %1 ").arg(m_currentTextForHighlights)) != -1)
        regex = QStringLiteral("%1\\b").arg(regex);
    QVector<KTextEditor::Range> matches;
    do {
        searchRange.setRange(start, visibleRange().end());

        matches = doc()->searchText(searchRange, regex, KTextEditor::Regex);

        if (matches.first().isValid()) {
            KTextEditor::MovingRange *mr = doc()->newMovingRange(matches.first());
            mr->setAttribute(attr);
            mr->setView(this);
            mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
            mr->setAttributeOnlyForViews(true);
            m_rangesForHighlights.append(mr);
            start = matches.first().end();
        }
    } while (matches.first().isValid());
}

KateAbstractInputMode *KTextEditor::ViewPrivate::currentInputMode() const
{
    return m_viewInternal->m_currentInputMode;
}

void KTextEditor::ViewPrivate::toggleInputMode()
{
    if (QAction *a = dynamic_cast<QAction *>(sender())) {
        setInputMode(static_cast<KTextEditor::View::InputMode>(a->data().toInt()));
    }
}

void KTextEditor::ViewPrivate::cycleInputMode()
{
    InputMode current = currentInputMode()->viewInputMode();
    InputMode to = (current == KTextEditor::View::NormalInputMode) ? KTextEditor::View::ViInputMode : KTextEditor::View::NormalInputMode;
    setInputMode(to);
}

// BEGIN KTextEditor::PrintInterface stuff
bool KTextEditor::ViewPrivate::print()
{
    return KatePrinter::print(this);
}

void KTextEditor::ViewPrivate::printPreview()
{
    KatePrinter::printPreview(this);
}

// END

// BEGIN KTextEditor::InlineNoteInterface
void KTextEditor::ViewPrivate::registerInlineNoteProvider(KTextEditor::InlineNoteProvider *provider)
{
    if (!m_inlineNoteProviders.contains(provider)) {
        m_inlineNoteProviders.append(provider);

        connect(provider, &KTextEditor::InlineNoteProvider::inlineNotesReset, this, &ViewPrivate::inlineNotesReset);
        connect(provider, &KTextEditor::InlineNoteProvider::inlineNotesChanged, this, &ViewPrivate::inlineNotesLineChanged);

        inlineNotesReset();
    }
}

void KTextEditor::ViewPrivate::unregisterInlineNoteProvider(KTextEditor::InlineNoteProvider *provider)
{
    const int index = m_inlineNoteProviders.indexOf(provider);
    if (index >= 0) {
        m_inlineNoteProviders.removeAt(index);

        provider->disconnect(this);

        inlineNotesReset();
    }
}

QVarLengthArray<KateInlineNoteData, 8> KTextEditor::ViewPrivate::inlineNotes(int line) const
{
    QVarLengthArray<KateInlineNoteData, 8> allInlineNotes;
    for (KTextEditor::InlineNoteProvider *provider : m_inlineNoteProviders) {
        int index = 0;
        for (auto column : provider->inlineNotes(line)) {
            const bool underMouse = Cursor(line, column) == m_viewInternal->m_activeInlineNote.m_position;
            KateInlineNoteData note = {provider, this, {line, column}, index, underMouse, m_viewInternal->renderer()->currentFont(), m_viewInternal->renderer()->lineHeight()};
            allInlineNotes.append(note);
            index++;
        }
    }
    return allInlineNotes;
}

QRect KTextEditor::ViewPrivate::inlineNoteRect(const KateInlineNoteData &note) const
{
    return m_viewInternal->inlineNoteRect(note);
}

void KTextEditor::ViewPrivate::inlineNotesReset()
{
    m_viewInternal->m_activeInlineNote = {};
    tagLines(0, doc()->lastLine(), true);
}

void KTextEditor::ViewPrivate::inlineNotesLineChanged(int line)
{
    if (line == m_viewInternal->m_activeInlineNote.m_position.line()) {
        m_viewInternal->m_activeInlineNote = {};
    }
    tagLines(line, line, true);
}

// END KTextEditor::InlineNoteInterface

KTextEditor::Attribute::Ptr KTextEditor::ViewPrivate::defaultStyleAttribute(KTextEditor::DefaultStyle defaultStyle) const
{
    KateRendererConfig *renderConfig = const_cast<KTextEditor::ViewPrivate *>(this)->renderer()->config();

    KTextEditor::Attribute::Ptr style = doc()->highlight()->attributes(renderConfig->schema()).at(defaultStyle);
    if (!style->hasProperty(QTextFormat::BackgroundBrush)) {
        // make sure the returned style has the default background color set
        style = new KTextEditor::Attribute(*style);
        style->setBackground(QBrush(renderConfig->backgroundColor()));
    }
    return style;
}

QList<KTextEditor::AttributeBlock> KTextEditor::ViewPrivate::lineAttributes(int line)
{
    QList<KTextEditor::AttributeBlock> attribs;

    if (line < 0 || line >= doc()->lines())
        return attribs;

    Kate::TextLine kateLine = doc()->kateTextLine(line);
    if (!kateLine) {
        return attribs;
    }

    const QVector<Kate::TextLineData::Attribute> &intAttrs = kateLine->attributesList();
    for (int i = 0; i < intAttrs.size(); ++i) {
        if (intAttrs[i].length > 0 && intAttrs[i].attributeValue > 0) {
            attribs << KTextEditor::AttributeBlock(intAttrs.at(i).offset, intAttrs.at(i).length, renderer()->attribute(intAttrs.at(i).attributeValue));
        }
    }

    return attribs;
}

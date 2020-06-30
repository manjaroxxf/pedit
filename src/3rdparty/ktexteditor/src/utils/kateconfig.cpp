/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2007, 2008 Matthew Woehlke <mw_triad@users.sourceforge.net>
    Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>

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

#include "kateconfig.h"

#include "katedefaultcolors.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katepartdebug.h"
#include "katerenderer.h"
#include "kateschema.h"
#include "kateview.h"

#include <math.h>

#include <KCharsets>
#include <KConfigGroup>

#include <QSettings>
#include <QStringListModel>
#include <QTextCodec>

// BEGIN KateConfig
KateConfig::KateConfig(const KateConfig *parent)
    : m_parent(parent)
    , m_configKeys(m_parent ? nullptr : new QStringList())
    , m_configKeyToEntry(m_parent ? nullptr : new QHash<QString, const ConfigEntry *>())
{
}

KateConfig::~KateConfig()
{
}

void KateConfig::addConfigEntry(ConfigEntry &&entry)
{
    /**
     * shall only be called for toplevel config
     */
    Q_ASSERT(isGlobal());

    /**
     * there shall be no gaps in the entries
     * we might later want to use a vector
     */
    Q_ASSERT(m_configEntries.size() == static_cast<size_t>(entry.enumKey));

    /**
     * add new element
     */
    m_configEntries.emplace(entry.enumKey, entry);
}

void KateConfig::finalizeConfigEntries()
{
    /**
     * shall only be called for toplevel config
     */
    Q_ASSERT(isGlobal());

    /**
     * compute list of all config keys + register map from key => config entry
     *
     * we skip entries without a command name, these config entries are not exposed ATM
     *
     */
    for (const auto &entry : m_configEntries) {
        if (!entry.second.commandName.isEmpty()) {
            Q_ASSERT_X(!m_configKeys->contains(entry.second.commandName), "finalizeConfigEntries", (QLatin1String("KEY NOT UNIQUE: ") + entry.second.commandName).toLocal8Bit().constData());
            m_configKeys->append(entry.second.commandName);
            m_configKeyToEntry->insert(entry.second.commandName, &entry.second);
        }
    }
}

void KateConfig::readConfigEntries(const KConfigGroup &config)
{
    configStart();

    // read all config entries, even the ones ATM not set in this config object but known in the toplevel one
    for (const auto &entry : fullConfigEntries()) {
        setValue(entry.second.enumKey, config.readEntry(entry.second.configKey, entry.second.defaultValue));
    }

    configEnd();
}

void KateConfig::writeConfigEntries(KConfigGroup &config) const
{
    // write all config entries, even the ones ATM not set in this config object but known in the toplevel one
    for (const auto &entry : fullConfigEntries()) {
        config.writeEntry(entry.second.configKey, value(entry.second.enumKey));
    }
}

void KateConfig::configStart()
{
    configSessionNumber++;

    if (configSessionNumber > 1) {
        return;
    }

    configIsRunning = true;
}

void KateConfig::configEnd()
{
    if (configSessionNumber == 0) {
        return;
    }

    configSessionNumber--;

    if (configSessionNumber > 0) {
        return;
    }

    configIsRunning = false;

    updateConfig();
}

QVariant KateConfig::value(const int key) const
{
    // first: local lookup
    const auto it = m_configEntries.find(key);
    if (it != m_configEntries.end()) {
        return it->second.value;
    }

    // else: fallback to parent config, if any
    if (m_parent) {
        return m_parent->value(key);
    }

    // if we arrive here, the key was invalid! => programming error
    // for release builds, we just return invalid variant
    Q_ASSERT(false);
    return QVariant();
}

bool KateConfig::setValue(const int key, const QVariant &value)
{
    // check: is this key known at all?
    const auto &knownEntries = fullConfigEntries();
    const auto knownIt = knownEntries.find(key);
    if (knownIt == knownEntries.end()) {
        // if we arrive here, the key was invalid! => programming error
        // for release builds, we just fail to set the value
        Q_ASSERT(false);
        return false;
    }

    // validator set? use it, if not accepting, abort setting
    if (knownIt->second.validator && !knownIt->second.validator(value)) {
        return false;
    }

    // check if value already there for this config
    auto valueIt = m_configEntries.find(key);
    if (valueIt != m_configEntries.end()) {
        // skip any work if value is equal
        if (valueIt->second.value == value) {
            return true;
        }

        // else: alter value and be done
        configStart();
        valueIt->second.value = value;
        configEnd();
        return true;
    }

    // if not in this hash, we must copy the known entry and adjust the value
    configStart();
    auto res = m_configEntries.emplace(key, knownIt->second);
    res.first->second.value = value;
    configEnd();
    return true;
}

QVariant KateConfig::value(const QString &key) const
{
    /**
     * check if we know this key, if not, return invalid variant
     */
    const auto &knownEntries = fullConfigKeyToEntry();
    const auto it = knownEntries.find(key);
    if (it == knownEntries.end()) {
        return QVariant();
    }

    /**
     * key known, dispatch to normal value() function with enum
     */
    return value(it.value()->enumKey);
}

bool KateConfig::setValue(const QString &key, const QVariant &value)
{
    /**
     * check if we know this key, if not, ignore the set
     */
    const auto &knownEntries = fullConfigKeyToEntry();
    const auto it = knownEntries.find(key);
    if (it == knownEntries.end()) {
        return false;
    }

    /**
     * key known, dispatch to normal setValue() function with enum
     */
    return setValue(it.value()->enumKey, value);
}

// END

// BEGIN HelperFunctions
KateGlobalConfig *KateGlobalConfig::s_global = nullptr;
KateDocumentConfig *KateDocumentConfig::s_global = nullptr;
KateViewConfig *KateViewConfig::s_global = nullptr;
KateRendererConfig *KateRendererConfig::s_global = nullptr;

/**
 * validate if an encoding is ok
 * @param name encoding name
 * @return encoding ok?
 */
static bool isEncodingOk(const QString &name)
{
    bool found = false;
    auto codec = KCharsets::charsets()->codecForName(name, found);
    return found && codec;
}

static bool inBounds(const int min, const QVariant &value, const int max)
{
    const int val = value.toInt();
    return (val >= min) && (val <= max);
}

static bool isPositive(const QVariant &value)
{
    bool ok;
    value.toUInt(&ok);
    return ok;
}
// END

// BEGIN KateGlobalConfig
KateGlobalConfig::KateGlobalConfig()
{
    /**
     * register this as our global instance
     */
    Q_ASSERT(isGlobal());
    s_global = this;

    /**
     * init all known config entries
     */
    addConfigEntry(ConfigEntry(EncodingProberType, "Encoding Prober Type", QString(), KEncodingProber::Universal));
    addConfigEntry(ConfigEntry(FallbackEncoding, "Fallback Encoding", QString(), QStringLiteral("ISO 8859-15"), [](const QVariant &value) { return isEncodingOk(value.toString()); }));

    /**
     * finalize the entries, e.g. hashs them
     */
    finalizeConfigEntries();

    /**
     * init with defaults from config or really hardcoded ones
     */
    KConfigGroup cg(KTextEditor::EditorPrivate::config(), "KTextEditor Editor");
    readConfig(cg);
}

void KateGlobalConfig::readConfig(const KConfigGroup &config)
{
    /**
     * start config update group
     */
    configStart();

    /**
     * read generic entries
     */
    readConfigEntries(config);

    /**
     * end config update group, might trigger updateConfig()
     */
    configEnd();
}

void KateGlobalConfig::writeConfig(KConfigGroup &config)
{
    /**
     * write generic entries
     */
    writeConfigEntries(config);
}

void KateGlobalConfig::updateConfig()
{
    // write config
    KConfigGroup cg(KTextEditor::EditorPrivate::config(), "KTextEditor Editor");
    writeConfig(cg);
    KTextEditor::EditorPrivate::config()->sync();
}

QTextCodec *KateGlobalConfig::fallbackCodec() const
{
    /**
     * query stored encoding, always fallback to ISO 8859-15 if nothing valid set
     */
    const auto encoding = value(FallbackEncoding).toString();
    if (encoding.isEmpty()) {
        return QTextCodec::codecForName("ISO 8859-15");
    }

    /**
     * use configured encoding
     */
    return KCharsets::charsets()->codecForName(encoding);
}
// END

// BEGIN KateDocumentConfig
KateDocumentConfig::KateDocumentConfig()
{
    /**
     * register this as our global instance
     */
    Q_ASSERT(isGlobal());
    s_global = this;

    /**
     * init all known config entries
     */
    addConfigEntry(ConfigEntry(TabWidth, "Tab Width", QStringLiteral("tab-width"), 4, [](const QVariant &value) { return value.toInt() >= 1; }));
    addConfigEntry(ConfigEntry(IndentationWidth, "Indentation Width", QStringLiteral("indent-width"), 4, [](const QVariant &value) { return value.toInt() >= 1; }));
    addConfigEntry(ConfigEntry(OnTheFlySpellCheck, "On-The-Fly Spellcheck", QStringLiteral("on-the-fly-spellcheck"), false));
    addConfigEntry(ConfigEntry(IndentOnTextPaste, "Indent On Text Paste", QStringLiteral("indent-pasted-text"), false));
    addConfigEntry(ConfigEntry(ReplaceTabsWithSpaces, "ReplaceTabsDyn", QStringLiteral("replace-tabs"), true));
    addConfigEntry(ConfigEntry(BackupOnSaveLocal, "Backup Local", QStringLiteral("backup-on-save-local"), false));
    addConfigEntry(ConfigEntry(BackupOnSaveRemote, "Backup Remote", QStringLiteral("backup-on-save-remote"), false));
    addConfigEntry(ConfigEntry(BackupOnSavePrefix, "Backup Prefix", QStringLiteral("backup-on-save-prefix"), QString()));
    addConfigEntry(ConfigEntry(BackupOnSaveSuffix, "Backup Suffix", QStringLiteral("backup-on-save-suffix"), QStringLiteral("~")));
    addConfigEntry(ConfigEntry(IndentationMode, "Indentation Mode", QString(), QStringLiteral("normal")));
    addConfigEntry(ConfigEntry(TabHandlingMode, "Tab Handling", QString(), KateDocumentConfig::tabSmart));
    addConfigEntry(ConfigEntry(StaticWordWrap, "Word Wrap", QString(), false));
    addConfigEntry(ConfigEntry(StaticWordWrapColumn, "Word Wrap Column", QString(), 80, [](const QVariant &value) { return value.toInt() >= 1; }));
    addConfigEntry(ConfigEntry(PageUpDownMovesCursor, "PageUp/PageDown Moves Cursor", QString(), false));
    addConfigEntry(ConfigEntry(SmartHome, "Smart Home", QString(), true));
    addConfigEntry(ConfigEntry(ShowTabs, "Show Tabs", QString(), true));
    addConfigEntry(ConfigEntry(IndentOnTab, "Indent On Tab", QString(), true));
    addConfigEntry(ConfigEntry(KeepExtraSpaces, "Keep Extra Spaces", QString(), false));
    addConfigEntry(ConfigEntry(BackspaceIndents, "Indent On Backspace", QString(), true));
    addConfigEntry(ConfigEntry(ShowSpacesMode, "Show Spaces", QString(), KateDocumentConfig::None));
    addConfigEntry(ConfigEntry(TrailingMarkerSize, "Trailing Marker Size", QString(), 1));
    addConfigEntry(ConfigEntry(RemoveSpacesMode, "Remove Spaces", QString(), 0));
    addConfigEntry(ConfigEntry(NewlineAtEOF, "Newline at End of File", QString(), true));
    addConfigEntry(ConfigEntry(OverwriteMode, "Overwrite Mode", QString(), false));
    addConfigEntry(ConfigEntry(Encoding, "Encoding", QString(), QStringLiteral("UTF-8"), [](const QVariant &value) { return isEncodingOk(value.toString()); }));
    addConfigEntry(ConfigEntry(EndOfLine, "End of Line", QString(), 0));
    addConfigEntry(ConfigEntry(AllowEndOfLineDetection, "Allow End of Line Detection", QString(), true));
    addConfigEntry(ConfigEntry(ByteOrderMark, "BOM", QString(), false));
    addConfigEntry(ConfigEntry(SwapFile, "Swap File Mode", QString(), KateDocumentConfig::EnableSwapFile));
    addConfigEntry(ConfigEntry(SwapFileDirectory, "Swap Directory", QString(), QString()));
    addConfigEntry(ConfigEntry(SwapFileSyncInterval, "Swap Sync Interval", QString(), 15));
    addConfigEntry(ConfigEntry(LineLengthLimit, "Line Length Limit", QString(), 10000));

    /**
     * finalize the entries, e.g. hashs them
     */
    finalizeConfigEntries();

    /**
     * init with defaults from config or really hardcoded ones
     */
    KConfigGroup cg(KTextEditor::EditorPrivate::config(), "KTextEditor Document");
    readConfig(cg);
}

KateDocumentConfig::KateDocumentConfig(KTextEditor::DocumentPrivate *doc)
    : KateConfig(s_global)
    , m_doc(doc)
{
    /**
     * per document config doesn't read stuff per default
     */
}

void KateDocumentConfig::readConfig(const KConfigGroup &config)
{
    /**
     * start config update group
     */
    configStart();

    /**
     * read generic entries
     */
    readConfigEntries(config);

    /**
     * fixup sonnet config, see KateSpellCheckConfigTab::apply(), too
     * WARNING: this is slightly hackish, but it's currently the only way to
     *          do it, see also the KTextEdit class
     */
    if (isGlobal()) {
        const QSettings settings(QStringLiteral("KDE"), QStringLiteral("Sonnet"));
        setOnTheFlySpellCheck(settings.value(QStringLiteral("checkerEnabledByDefault"), false).toBool());
    }

    /**
     * backwards compatibility mappings
     * convert stuff, old entries deleted in writeConfig
     */
    if (const int backupFlags = config.readEntry("Backup Flags", 0)) {
        setBackupOnSaveLocal(backupFlags & 0x1);
        setBackupOnSaveRemote(backupFlags & 0x2);
    }

    /**
     * end config update group, might trigger updateConfig()
     */
    configEnd();
}

void KateDocumentConfig::writeConfig(KConfigGroup &config)
{
    /**
     * write generic entries
     */
    writeConfigEntries(config);

    /**
     * backwards compatibility mappings
     * here we remove old entries we converted on readConfig
     */
    config.deleteEntry("Backup Flags");
}

void KateDocumentConfig::updateConfig()
{
    if (m_doc) {
        m_doc->updateConfig();
        return;
    }

    if (isGlobal()) {
        for (int z = 0; z < KTextEditor::EditorPrivate::self()->kateDocuments().size(); ++z) {
            (KTextEditor::EditorPrivate::self()->kateDocuments())[z]->updateConfig();
        }

        // write config
        KConfigGroup cg(KTextEditor::EditorPrivate::config(), "KTextEditor Document");
        writeConfig(cg);
        KTextEditor::EditorPrivate::config()->sync();
    }
}

QTextCodec *KateDocumentConfig::codec() const
{
    /**
     * query stored encoding, always fallback to UTF-8 if nothing valid set
     */
    const auto encoding = value(Encoding).toString();
    if (encoding.isEmpty()) {
        return QTextCodec::codecForName("UTF-8");
    }

    /**
     * use configured encoding
     */
    return KCharsets::charsets()->codecForName(encoding);
}

QString KateDocumentConfig::eolString()
{
    switch (eol()) {
    case KateDocumentConfig::eolDos:
        return QStringLiteral("\r\n");

    case KateDocumentConfig::eolMac:
        return QStringLiteral("\r");

    default:
        return QStringLiteral("\n");
    }
}
// END

// BEGIN KateViewConfig
KateViewConfig::KateViewConfig()
{
    s_global = this;

    // Init all known config entries
    // NOTE: Ensure to keep the same order as listed in enum ConfigEntryTypes or it will later assert!
    // addConfigEntry(ConfigEntry(<EnumKey>, <ConfigKey>, <CommandName>, <DefaultValue>,  [<ValidatorFunction>]))
    addConfigEntry(ConfigEntry(AllowMarkMenu, "Allow Mark Menu", QStringLiteral("allow-mark-menu"), true));
    addConfigEntry(ConfigEntry(AutoBrackets, "Auto Brackets", QStringLiteral("auto-brackets"), false));
    addConfigEntry(ConfigEntry(AutoCenterLines, "Auto Center Lines", QStringLiteral("auto-center-lines"), 0));
    addConfigEntry(ConfigEntry(AutomaticCompletionInvocation, "Auto Completion", QString(), true));
    addConfigEntry(ConfigEntry(BackspaceRemoveComposedCharacters, "Backspace Remove Composed Characters", QString(), false));
    addConfigEntry(ConfigEntry(BookmarkSorting, "Bookmark Menu Sorting", QString(), 0));
    addConfigEntry(ConfigEntry(CharsToEncloseSelection, "Chars To Enclose Selection", QStringLiteral("enclose-selection"), QString()));
    addConfigEntry(ConfigEntry(DefaultMarkType, "Default Mark Type", QStringLiteral("default-mark-type"), KTextEditor::MarkInterface::markType01, [](const QVariant &value) { return isPositive(value); }));
    addConfigEntry(ConfigEntry(DynWordWrapAlignIndent, "Dynamic Word Wrap Align Indent", QString(), 80, [](const QVariant &value) { return inBounds(1, value, 100); }));
    addConfigEntry(ConfigEntry(DynWordWrapIndicators, "Dynamic Word Wrap Indicators", QString(), 1, [](const QVariant &value) { return inBounds(1, value, 3); }));
    addConfigEntry(ConfigEntry(DynWrapAnywhere, "Dynamic Wrap not at word boundaries", QStringLiteral("dynamic-word-wrap-anywhere"), false));
    addConfigEntry(ConfigEntry(DynWrapAtStaticMarker, "Dynamic Word Wrap At Static Marker", QString(), false));
    addConfigEntry(ConfigEntry(DynamicWordWrap, "Dynamic Word Wrap", QStringLiteral("dynamic-word-wrap"), true));
    addConfigEntry(ConfigEntry(FoldFirstLine, "Fold First Line", QString(), false));
    addConfigEntry(ConfigEntry(InputMode, "Input Mode", QString(), 0, [](const QVariant &value) { return isPositive(value); }));
    addConfigEntry(ConfigEntry(KeywordCompletion, "Keyword Completion", QStringLiteral("keyword-completion"), true));
    addConfigEntry(ConfigEntry(MaxHistorySize, "Maximum Search History Size", QString(), 100, [](const QVariant &value) { return inBounds(0, value, 999); }));
    addConfigEntry(ConfigEntry(MousePasteAtCursorPosition, "Mouse Paste At Cursor Position", QString(), false));
    addConfigEntry(ConfigEntry(PersistentSelection, "Persistent Selection", QStringLiteral("persistent-selectionq"), false));
    addConfigEntry(ConfigEntry(ScrollBarMiniMapWidth, "Scroll Bar Mini Map Width", QString(), 60, [](const QVariant &value) { return inBounds(0, value, 999); }));
    addConfigEntry(ConfigEntry(ScrollPastEnd, "Scroll Past End", QString(), false));
    addConfigEntry(ConfigEntry(SearchFlags, "Search/Replace Flags", QString(), IncFromCursor | PowerMatchCase | PowerModePlainText));
    addConfigEntry(ConfigEntry(ShowFoldingBar, "Folding Bar", QStringLiteral("folding-bar"), true));
    addConfigEntry(ConfigEntry(ShowFoldingPreview, "Folding Preview", QStringLiteral("folding-preview"), true));
    addConfigEntry(ConfigEntry(ShowIconBar, "Icon Bar", QStringLiteral("icon-bar"), false));
    addConfigEntry(ConfigEntry(ShowLineCount, "Show Line Count", QString(), false));
    addConfigEntry(ConfigEntry(ShowLineModification, "Line Modification", QStringLiteral("modification-markers"), false));
    addConfigEntry(ConfigEntry(ShowLineNumbers, "Line Numbers", QStringLiteral("line-numbers"), false));
    addConfigEntry(ConfigEntry(ShowScrollBarMarks, "Scroll Bar Marks", QString(), false));
    addConfigEntry(ConfigEntry(ShowScrollBarMiniMap, "Scroll Bar MiniMap", QStringLiteral("scrollbar-minimap"), true));
    addConfigEntry(ConfigEntry(ShowScrollBarMiniMapAll, "Scroll Bar Mini Map All", QString(), true));
    addConfigEntry(ConfigEntry(ShowScrollBarPreview, "Scroll Bar Preview", QStringLiteral("scrollbar-preview"), true));
    addConfigEntry(ConfigEntry(ShowScrollbars, "Show Scrollbars", QString(), AlwaysOn, [](const QVariant &value) { return inBounds(0, value, 2); }));
    addConfigEntry(ConfigEntry(ShowWordCount, "Show Word Count", QString(), false));
    addConfigEntry(ConfigEntry(TextDragAndDrop, "Text Drag And Drop", QString(), true));
    addConfigEntry(ConfigEntry(SmartCopyCut, "Smart Copy Cut", QString(), false));
    addConfigEntry(ConfigEntry(UserSetsOfCharsToEncloseSelection, "User Sets Of Chars To Enclose Selection", QString(), QStringList()));
    addConfigEntry(ConfigEntry(ViInputModeStealKeys, "Vi Input Mode Steal Keys", QString(), false));
    addConfigEntry(ConfigEntry(ViRelativeLineNumbers, "Vi Relative Line Numbers", QString(), false));
    addConfigEntry(ConfigEntry(WordCompletion, "Word Completion", QString(), true));
    addConfigEntry(ConfigEntry(WordCompletionMinimalWordLength, "Word Completion Minimal Word Length", QString(), 3, [](const QVariant &value) { return inBounds(0, value, 99); }));
    addConfigEntry(ConfigEntry(WordCompletionRemoveTail, "Word Completion Remove Tail", QString(), true));

    // Never forget to finalize or the <CommandName> becomes not available
    finalizeConfigEntries();

    // init with defaults from config or really hardcoded ones
    KConfigGroup config(KTextEditor::EditorPrivate::config(), "KTextEditor View");
    readConfig(config);
}

KateViewConfig::KateViewConfig(KTextEditor::ViewPrivate *view)
    : KateConfig(s_global)
    , m_view(view)
{
}

KateViewConfig::~KateViewConfig()
{
}

void KateViewConfig::readConfig(const KConfigGroup &config)
{
    configStart();

    // read generic entries
    readConfigEntries(config);

    configEnd();
}

void KateViewConfig::writeConfig(KConfigGroup &config)
{
    // write generic entries
    writeConfigEntries(config);
}

void KateViewConfig::updateConfig()
{
    if (m_view) {
        m_view->updateConfig();
        return;
    }

    if (isGlobal()) {
        const auto allViews = KTextEditor::EditorPrivate::self()->views();
        for (KTextEditor::ViewPrivate *view : allViews) {
            view->updateConfig();
        }

        // write config
        KConfigGroup cg(KTextEditor::EditorPrivate::config(), "KTextEditor View");
        writeConfig(cg);
        KTextEditor::EditorPrivate::config()->sync();
    }
}
// END

// BEGIN KateRendererConfig
KateRendererConfig::KateRendererConfig()
    : m_lineMarkerColor(KTextEditor::MarkInterface::reservedMarkersCount())
    ,

    m_schemaSet(false)
    , m_fontSet(false)
    , m_wordWrapMarkerSet(false)
    , m_showIndentationLinesSet(false)
    , m_showWholeBracketExpressionSet(false)
    , m_backgroundColorSet(false)
    , m_selectionColorSet(false)
    , m_highlightedLineColorSet(false)
    , m_highlightedBracketColorSet(false)
    , m_wordWrapMarkerColorSet(false)
    , m_tabMarkerColorSet(false)
    , m_indentationLineColorSet(false)
    , m_iconBarColorSet(false)
    , m_foldingColorSet(false)
    , m_lineNumberColorSet(false)
    , m_currentLineNumberColorSet(false)
    , m_separatorColorSet(false)
    , m_spellingMistakeLineColorSet(false)
    , m_templateColorsSet(false)
    , m_modifiedLineColorSet(false)
    , m_savedLineColorSet(false)
    , m_searchHighlightColorSet(false)
    , m_replaceHighlightColorSet(false)
    , m_lineMarkerColorSet(m_lineMarkerColor.size())

{
    // init bitarray
    m_lineMarkerColorSet.fill(true);

    s_global = this;

    // init with defaults from config or really hardcoded ones
    KConfigGroup config(KTextEditor::EditorPrivate::config(), "KTextEditor Renderer");
    readConfig(config);
}

KateRendererConfig::KateRendererConfig(KateRenderer *renderer)
    : KateConfig(s_global)
    , m_lineMarkerColor(KTextEditor::MarkInterface::reservedMarkersCount())
    , m_schemaSet(false)
    , m_fontSet(false)
    , m_wordWrapMarkerSet(false)
    , m_showIndentationLinesSet(false)
    , m_showWholeBracketExpressionSet(false)
    , m_backgroundColorSet(false)
    , m_selectionColorSet(false)
    , m_highlightedLineColorSet(false)
    , m_highlightedBracketColorSet(false)
    , m_wordWrapMarkerColorSet(false)
    , m_tabMarkerColorSet(false)
    , m_indentationLineColorSet(false)
    , m_iconBarColorSet(false)
    , m_foldingColorSet(false)
    , m_lineNumberColorSet(false)
    , m_currentLineNumberColorSet(false)
    , m_separatorColorSet(false)
    , m_spellingMistakeLineColorSet(false)
    , m_templateColorsSet(false)
    , m_modifiedLineColorSet(false)
    , m_savedLineColorSet(false)
    , m_searchHighlightColorSet(false)
    , m_replaceHighlightColorSet(false)
    , m_lineMarkerColorSet(m_lineMarkerColor.size())
    , m_renderer(renderer)
{
    // init bitarray
    m_lineMarkerColorSet.fill(false);
}

KateRendererConfig::~KateRendererConfig()
{
}

namespace
{
const char KEY_SCHEMA[] = "Schema";
const char KEY_WORD_WRAP_MARKER[] = "Word Wrap Marker";
const char KEY_SHOW_INDENTATION_LINES[] = "Show Indentation Lines";
const char KEY_SHOW_WHOLE_BRACKET_EXPRESSION[] = "Show Whole Bracket Expression";
const char KEY_ANIMATE_BRACKET_MATCHING[] = "Animate Bracket Matching";
}

void KateRendererConfig::readConfig(const KConfigGroup &config)
{
    configStart();

    // read generic entries
    readConfigEntries(config);

    // "Normal" Schema MUST BE THERE, see global kateschemarc
    setSchema(config.readEntry(KEY_SCHEMA, "Normal"));

    setWordWrapMarker(config.readEntry(KEY_WORD_WRAP_MARKER, false));

    setShowIndentationLines(config.readEntry(KEY_SHOW_INDENTATION_LINES, false));

    setShowWholeBracketExpression(config.readEntry(KEY_SHOW_WHOLE_BRACKET_EXPRESSION, false));

    setAnimateBracketMatching(config.readEntry(KEY_ANIMATE_BRACKET_MATCHING, false));

    configEnd();
}

void KateRendererConfig::writeConfig(KConfigGroup &config)
{
    // write generic entries
    writeConfigEntries(config);

    config.writeEntry(KEY_SCHEMA, schema());

    config.writeEntry(KEY_WORD_WRAP_MARKER, wordWrapMarker());

    config.writeEntry(KEY_SHOW_INDENTATION_LINES, showIndentationLines());

    config.writeEntry(KEY_SHOW_WHOLE_BRACKET_EXPRESSION, showWholeBracketExpression());

    config.writeEntry(KEY_ANIMATE_BRACKET_MATCHING, animateBracketMatching());
}

void KateRendererConfig::updateConfig()
{
    if (m_renderer) {
        m_renderer->updateConfig();
        return;
    }

    if (isGlobal()) {
        for (int z = 0; z < KTextEditor::EditorPrivate::self()->views().size(); ++z) {
            (KTextEditor::EditorPrivate::self()->views())[z]->renderer()->updateConfig();
        }

        // write config
        KConfigGroup cg(KTextEditor::EditorPrivate::config(), "KTextEditor Renderer");
        writeConfig(cg);
        KTextEditor::EditorPrivate::config()->sync();
    }
}

const QString &KateRendererConfig::schema() const
{
    if (m_schemaSet || isGlobal()) {
        return m_schema;
    }

    return s_global->schema();
}

void KateRendererConfig::setSchema(const QString &schema)
{
    if (m_schemaSet && m_schema == schema) {
        return;
    }

    configStart();
    m_schemaSet = true;
    m_schema = schema;
    setSchemaInternal(schema);
    configEnd();
}

void KateRendererConfig::reloadSchema()
{
    if (isGlobal()) {
        setSchemaInternal(m_schema);
        const auto allViews = KTextEditor::EditorPrivate::self()->views();
        for (KTextEditor::ViewPrivate *view : allViews) {
            view->renderer()->config()->reloadSchema();
        }
    }

    else if (m_renderer && m_schemaSet) {
        setSchemaInternal(m_schema);
    }

    // trigger renderer/view update
    if (m_renderer) {
        m_renderer->updateConfig();
    }
}

void KateRendererConfig::setSchemaInternal(const QString &schema)
{
    m_schemaSet = true;
    m_schema = schema;

    KConfigGroup config = KTextEditor::EditorPrivate::self()->schemaManager()->schema(schema);

    // use global color instance, creation is expensive!
    const KateDefaultColors &colors(KTextEditor::EditorPrivate::self()->defaultColors());

    m_backgroundColor = config.readEntry("Color Background", colors.color(Kate::Background));

    m_backgroundColorSet = true;
    m_selectionColor = config.readEntry("Color Selection", colors.color(Kate::SelectionBackground));
    m_selectionColorSet = true;
    m_highlightedLineColor = config.readEntry("Color Highlighted Line", colors.color(Kate::HighlightedLineBackground));
    m_highlightedLineColorSet = true;
    m_highlightedBracketColor = config.readEntry("Color Highlighted Bracket", colors.color(Kate::HighlightedBracket));
    m_highlightedBracketColorSet = true;
    m_wordWrapMarkerColor = config.readEntry("Color Word Wrap Marker", colors.color(Kate::WordWrapMarker));
    m_wordWrapMarkerColorSet = true;
    m_tabMarkerColor = config.readEntry("Color Tab Marker", colors.color(Kate::TabMarker));
    m_tabMarkerColorSet = true;
    m_indentationLineColor = config.readEntry("Color Indentation Line", colors.color(Kate::IndentationLine));
    m_indentationLineColorSet = true;
    m_iconBarColor = config.readEntry("Color Icon Bar", colors.color(Kate::IconBar));
    m_iconBarColorSet = true;
    m_foldingColor = config.readEntry("Color Code Folding", colors.color(Kate::CodeFolding));
    m_foldingColorSet = true;
    m_lineNumberColor = config.readEntry("Color Line Number", colors.color(Kate::LineNumber));
    m_lineNumberColorSet = true;
    m_currentLineNumberColor = config.readEntry("Color Current Line Number", colors.color(Kate::CurrentLineNumber));
    m_currentLineNumberColorSet = true;
    m_separatorColor = config.readEntry("Color Separator", colors.color(Kate::Separator));
    m_separatorColorSet = true;
    m_spellingMistakeLineColor = config.readEntry("Color Spelling Mistake Line", colors.color(Kate::SpellingMistakeLine));
    m_spellingMistakeLineColorSet = true;

    m_modifiedLineColor = config.readEntry("Color Modified Lines", colors.color(Kate::ModifiedLine));
    m_modifiedLineColorSet = true;
    m_savedLineColor = config.readEntry("Color Saved Lines", colors.color(Kate::SavedLine));
    m_savedLineColorSet = true;
    m_searchHighlightColor = config.readEntry("Color Search Highlight", colors.color(Kate::SearchHighlight));
    m_searchHighlightColorSet = true;
    m_replaceHighlightColor = config.readEntry("Color Replace Highlight", colors.color(Kate::ReplaceHighlight));
    m_replaceHighlightColorSet = true;

    for (int i = Kate::FIRST_MARK; i <= Kate::LAST_MARK; i++) {
        QColor col = config.readEntry(QStringLiteral("Color MarkType %1").arg(i + 1), colors.mark(i));
        m_lineMarkerColorSet[i] = true;
        m_lineMarkerColor[i] = col;
    }

    setFontWithDroppedStyleName(config.readEntry("Font", QFontDatabase::systemFont(QFontDatabase::FixedFont)));

    m_templateBackgroundColor = config.readEntry(QStringLiteral("Color Template Background"), colors.color(Kate::TemplateBackground));

    m_templateFocusedEditablePlaceholderColor = config.readEntry(QStringLiteral("Color Template Focused Editable Placeholder"), colors.color(Kate::TemplateFocusedEditablePlaceholder));

    m_templateEditablePlaceholderColor = config.readEntry(QStringLiteral("Color Template Editable Placeholder"), colors.color(Kate::TemplateEditablePlaceholder));

    m_templateNotEditablePlaceholderColor = config.readEntry(QStringLiteral("Color Template Not Editable Placeholder"), colors.color(Kate::TemplateNotEditablePlaceholder));

    m_templateColorsSet = true;
}

const QFont &KateRendererConfig::baseFont() const
{
    if (m_fontSet || isGlobal()) {
        return m_font;
    }

    return s_global->baseFont();
}

void KateRendererConfig::setFont(const QFont &font)
{
    if (m_fontSet && m_font == font) {
        return;
    }

    configStart();
    setFontWithDroppedStyleName(font);
    configEnd();
}

void KateRendererConfig::setFontWithDroppedStyleName(const QFont &font)
{
    /**
     * Drop styleName, otherwise stuff like bold/italic/... won't work as style!
     */
    m_font = font;
    m_font.setStyleName(QString());
    m_fontSet = true;
}

bool KateRendererConfig::wordWrapMarker() const
{
    if (m_wordWrapMarkerSet || isGlobal()) {
        return m_wordWrapMarker;
    }

    return s_global->wordWrapMarker();
}

void KateRendererConfig::setWordWrapMarker(bool on)
{
    if (m_wordWrapMarkerSet && m_wordWrapMarker == on) {
        return;
    }

    configStart();

    m_wordWrapMarkerSet = true;
    m_wordWrapMarker = on;

    configEnd();
}

const QColor &KateRendererConfig::backgroundColor() const
{
    if (m_backgroundColorSet || isGlobal()) {
        return m_backgroundColor;
    }

    return s_global->backgroundColor();
}

void KateRendererConfig::setBackgroundColor(const QColor &col)
{
    if (m_backgroundColorSet && m_backgroundColor == col) {
        return;
    }

    configStart();

    m_backgroundColorSet = true;
    m_backgroundColor = col;

    configEnd();
}

const QColor &KateRendererConfig::selectionColor() const
{
    if (m_selectionColorSet || isGlobal()) {
        return m_selectionColor;
    }

    return s_global->selectionColor();
}

void KateRendererConfig::setSelectionColor(const QColor &col)
{
    if (m_selectionColorSet && m_selectionColor == col) {
        return;
    }

    configStart();

    m_selectionColorSet = true;
    m_selectionColor = col;

    configEnd();
}

const QColor &KateRendererConfig::highlightedLineColor() const
{
    if (m_highlightedLineColorSet || isGlobal()) {
        return m_highlightedLineColor;
    }

    return s_global->highlightedLineColor();
}

void KateRendererConfig::setHighlightedLineColor(const QColor &col)
{
    if (m_highlightedLineColorSet && m_highlightedLineColor == col) {
        return;
    }

    configStart();

    m_highlightedLineColorSet = true;
    m_highlightedLineColor = col;

    configEnd();
}

const QColor &KateRendererConfig::lineMarkerColor(KTextEditor::MarkInterface::MarkTypes type) const
{
    int index = 0;
    if (type > 0) {
        while ((type >> index++) ^ 1) {
        }
    }
    index -= 1;

    if (index < 0 || index >= KTextEditor::MarkInterface::reservedMarkersCount()) {
        static QColor dummy;
        return dummy;
    }

    if (m_lineMarkerColorSet[index] || isGlobal()) {
        return m_lineMarkerColor[index];
    }

    return s_global->lineMarkerColor(type);
}

void KateRendererConfig::setLineMarkerColor(const QColor &col, KTextEditor::MarkInterface::MarkTypes type)
{
    int index = static_cast<int>(log(static_cast<double>(type)) / log(2.0));
    Q_ASSERT(index >= 0 && index < KTextEditor::MarkInterface::reservedMarkersCount());

    if (m_lineMarkerColorSet[index] && m_lineMarkerColor[index] == col) {
        return;
    }

    configStart();

    m_lineMarkerColorSet[index] = true;
    m_lineMarkerColor[index] = col;

    configEnd();
}

const QColor &KateRendererConfig::highlightedBracketColor() const
{
    if (m_highlightedBracketColorSet || isGlobal()) {
        return m_highlightedBracketColor;
    }

    return s_global->highlightedBracketColor();
}

void KateRendererConfig::setHighlightedBracketColor(const QColor &col)
{
    if (m_highlightedBracketColorSet && m_highlightedBracketColor == col) {
        return;
    }

    configStart();

    m_highlightedBracketColorSet = true;
    m_highlightedBracketColor = col;

    configEnd();
}

const QColor &KateRendererConfig::wordWrapMarkerColor() const
{
    if (m_wordWrapMarkerColorSet || isGlobal()) {
        return m_wordWrapMarkerColor;
    }

    return s_global->wordWrapMarkerColor();
}

void KateRendererConfig::setWordWrapMarkerColor(const QColor &col)
{
    if (m_wordWrapMarkerColorSet && m_wordWrapMarkerColor == col) {
        return;
    }

    configStart();

    m_wordWrapMarkerColorSet = true;
    m_wordWrapMarkerColor = col;

    configEnd();
}

const QColor &KateRendererConfig::tabMarkerColor() const
{
    if (m_tabMarkerColorSet || isGlobal()) {
        return m_tabMarkerColor;
    }

    return s_global->tabMarkerColor();
}

void KateRendererConfig::setTabMarkerColor(const QColor &col)
{
    if (m_tabMarkerColorSet && m_tabMarkerColor == col) {
        return;
    }

    configStart();

    m_tabMarkerColorSet = true;
    m_tabMarkerColor = col;

    configEnd();
}

const QColor &KateRendererConfig::indentationLineColor() const
{
    if (m_indentationLineColorSet || isGlobal()) {
        return m_indentationLineColor;
    }

    return s_global->indentationLineColor();
}

void KateRendererConfig::setIndentationLineColor(const QColor &col)
{
    if (m_indentationLineColorSet && m_indentationLineColor == col) {
        return;
    }

    configStart();

    m_indentationLineColorSet = true;
    m_indentationLineColor = col;

    configEnd();
}

const QColor &KateRendererConfig::iconBarColor() const
{
    if (m_iconBarColorSet || isGlobal()) {
        return m_iconBarColor;
    }

    return s_global->iconBarColor();
}

void KateRendererConfig::setIconBarColor(const QColor &col)
{
    if (m_iconBarColorSet && m_iconBarColor == col) {
        return;
    }

    configStart();

    m_iconBarColorSet = true;
    m_iconBarColor = col;

    configEnd();
}

const QColor &KateRendererConfig::foldingColor() const
{
    if (m_foldingColorSet || isGlobal()) {
        return m_foldingColor;
    }

    return s_global->foldingColor();
}

void KateRendererConfig::setFoldingColor(const QColor &col)
{
    if (m_foldingColorSet && m_foldingColor == col) {
        return;
    }

    configStart();

    m_foldingColorSet = true;
    m_foldingColor = col;

    configEnd();
}

const QColor &KateRendererConfig::templateBackgroundColor() const
{
    if (m_templateColorsSet || isGlobal()) {
        return m_templateBackgroundColor;
    }

    return s_global->templateBackgroundColor();
}

const QColor &KateRendererConfig::templateEditablePlaceholderColor() const
{
    if (m_templateColorsSet || isGlobal()) {
        return m_templateEditablePlaceholderColor;
    }

    return s_global->templateEditablePlaceholderColor();
}

const QColor &KateRendererConfig::templateFocusedEditablePlaceholderColor() const
{
    if (m_templateColorsSet || isGlobal()) {
        return m_templateFocusedEditablePlaceholderColor;
    }

    return s_global->templateFocusedEditablePlaceholderColor();
}

const QColor &KateRendererConfig::templateNotEditablePlaceholderColor() const
{
    if (m_templateColorsSet || isGlobal()) {
        return m_templateNotEditablePlaceholderColor;
    }

    return s_global->templateNotEditablePlaceholderColor();
}

const QColor &KateRendererConfig::lineNumberColor() const
{
    if (m_lineNumberColorSet || isGlobal()) {
        return m_lineNumberColor;
    }

    return s_global->lineNumberColor();
}

void KateRendererConfig::setLineNumberColor(const QColor &col)
{
    if (m_lineNumberColorSet && m_lineNumberColor == col) {
        return;
    }

    configStart();

    m_lineNumberColorSet = true;
    m_lineNumberColor = col;

    configEnd();
}

const QColor &KateRendererConfig::currentLineNumberColor() const
{
    if (m_currentLineNumberColorSet || isGlobal()) {
        return m_currentLineNumberColor;
    }

    return s_global->currentLineNumberColor();
}

void KateRendererConfig::setCurrentLineNumberColor(const QColor &col)
{
    if (m_currentLineNumberColorSet && m_currentLineNumberColor == col) {
        return;
    }

    configStart();

    m_currentLineNumberColorSet = true;
    m_currentLineNumberColor = col;

    configEnd();
}

const QColor &KateRendererConfig::separatorColor() const
{
    if (m_separatorColorSet || isGlobal()) {
        return m_separatorColor;
    }

    return s_global->separatorColor();
}

void KateRendererConfig::setSeparatorColor(const QColor &col)
{
    if (m_separatorColorSet && m_separatorColor == col) {
        return;
    }

    configStart();

    m_separatorColorSet = true;
    m_separatorColor = col;

    configEnd();
}

const QColor &KateRendererConfig::spellingMistakeLineColor() const
{
    if (m_spellingMistakeLineColorSet || isGlobal()) {
        return m_spellingMistakeLineColor;
    }

    return s_global->spellingMistakeLineColor();
}

void KateRendererConfig::setSpellingMistakeLineColor(const QColor &col)
{
    if (m_spellingMistakeLineColorSet && m_spellingMistakeLineColor == col) {
        return;
    }

    configStart();

    m_spellingMistakeLineColorSet = true;
    m_spellingMistakeLineColor = col;

    configEnd();
}

const QColor &KateRendererConfig::modifiedLineColor() const
{
    if (m_modifiedLineColorSet || isGlobal()) {
        return m_modifiedLineColor;
    }

    return s_global->modifiedLineColor();
}

void KateRendererConfig::setModifiedLineColor(const QColor &col)
{
    if (m_modifiedLineColorSet && m_modifiedLineColor == col) {
        return;
    }

    configStart();

    m_modifiedLineColorSet = true;
    m_modifiedLineColor = col;

    configEnd();
}

const QColor &KateRendererConfig::savedLineColor() const
{
    if (m_savedLineColorSet || isGlobal()) {
        return m_savedLineColor;
    }

    return s_global->savedLineColor();
}

void KateRendererConfig::setSavedLineColor(const QColor &col)
{
    if (m_savedLineColorSet && m_savedLineColor == col) {
        return;
    }

    configStart();

    m_savedLineColorSet = true;
    m_savedLineColor = col;

    configEnd();
}

const QColor &KateRendererConfig::searchHighlightColor() const
{
    if (m_searchHighlightColorSet || isGlobal()) {
        return m_searchHighlightColor;
    }

    return s_global->searchHighlightColor();
}

void KateRendererConfig::setSearchHighlightColor(const QColor &col)
{
    if (m_searchHighlightColorSet && m_searchHighlightColor == col) {
        return;
    }

    configStart();

    m_searchHighlightColorSet = true;
    m_searchHighlightColor = col;

    configEnd();
}

const QColor &KateRendererConfig::replaceHighlightColor() const
{
    if (m_replaceHighlightColorSet || isGlobal()) {
        return m_replaceHighlightColor;
    }

    return s_global->replaceHighlightColor();
}

void KateRendererConfig::setReplaceHighlightColor(const QColor &col)
{
    if (m_replaceHighlightColorSet && m_replaceHighlightColor == col) {
        return;
    }

    configStart();

    m_replaceHighlightColorSet = true;
    m_replaceHighlightColor = col;

    configEnd();
}

bool KateRendererConfig::showIndentationLines() const
{
    if (m_showIndentationLinesSet || isGlobal()) {
        return m_showIndentationLines;
    }

    return s_global->showIndentationLines();
}

void KateRendererConfig::setShowIndentationLines(bool on)
{
    if (m_showIndentationLinesSet && m_showIndentationLines == on) {
        return;
    }

    configStart();

    m_showIndentationLinesSet = true;
    m_showIndentationLines = on;

    configEnd();
}

bool KateRendererConfig::showWholeBracketExpression() const
{
    if (m_showWholeBracketExpressionSet || isGlobal()) {
        return m_showWholeBracketExpression;
    }

    return s_global->showWholeBracketExpression();
}

void KateRendererConfig::setShowWholeBracketExpression(bool on)
{
    if (m_showWholeBracketExpressionSet && m_showWholeBracketExpression == on) {
        return;
    }

    configStart();

    m_showWholeBracketExpressionSet = true;
    m_showWholeBracketExpression = on;

    configEnd();
}

bool KateRendererConfig::animateBracketMatching() const
{
    return s_global->m_animateBracketMatching;
}

void KateRendererConfig::setAnimateBracketMatching(bool on)
{
    if (!isGlobal()) {
        s_global->setAnimateBracketMatching(on);
    } else if (on != m_animateBracketMatching) {
        configStart();
        m_animateBracketMatching = on;
        configEnd();
    }
}

// END

/* This file is part of the KDE libraries
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
   Copyright (C) 2013 Andrey Matveyakin <a.matveyakin@gmail.com>

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

#include "katerenderer.h"

#include "inlinenotedata.h"
#include "katebuffer.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "katehighlight.h"
#include "katerenderrange.h"
#include "katetextlayout.h"
#include "kateview.h"

#include "ktexteditor/inlinenote.h"
#include "ktexteditor/inlinenoteprovider.h"

#include "katepartdebug.h"

#include <QBrush>
#include <QFont>
#include <QPainter>
#include <QRegularExpression>
#include <QStack>
#include <QTextLine>
#include <QtMath> // qCeil

static const QChar tabChar(QLatin1Char('\t'));
static const QChar spaceChar(QLatin1Char(' '));
static const QChar nbSpaceChar(0xa0); // non-breaking space

KateRenderer::KateRenderer(KTextEditor::DocumentPrivate *doc, Kate::TextFolding &folding, KTextEditor::ViewPrivate *view)
    : m_doc(doc)
    , m_folding(folding)
    , m_view(view)
    , m_tabWidth(m_doc->config()->tabWidth())
    , m_indentWidth(m_doc->config()->indentationWidth())
    , m_caretStyle(KateRenderer::Line)
    , m_drawCaret(true)
    , m_showSelections(true)
    , m_showTabs(true)
    , m_showSpaces(KateDocumentConfig::Trailing)
    , m_showNonPrintableSpaces(false)
    , m_printerFriendly(false)
    , m_config(new KateRendererConfig(this))
    , m_font(m_config->baseFont())
    , m_fontMetrics(m_font)
{
    updateAttributes();

    // initialize with a sane font height
    updateFontHeight();

    // make the proper calculation for markerSize
    updateMarkerSize();
}

KateRenderer::~KateRenderer()
{
    delete m_config;
}

void KateRenderer::updateAttributes()
{
    m_attributes = m_doc->highlight()->attributes(config()->schema());
}

KTextEditor::Attribute::Ptr KateRenderer::attribute(uint pos) const
{
    if (pos < (uint)m_attributes.count()) {
        return m_attributes[pos];
    }

    return m_attributes[0];
}

KTextEditor::Attribute::Ptr KateRenderer::specificAttribute(int context) const
{
    if (context >= 0 && context < m_attributes.count()) {
        return m_attributes[context];
    }

    return m_attributes[0];
}

void KateRenderer::setDrawCaret(bool drawCaret)
{
    m_drawCaret = drawCaret;
}

void KateRenderer::setCaretStyle(KateRenderer::caretStyles style)
{
    m_caretStyle = style;
}

void KateRenderer::setShowTabs(bool showTabs)
{
    m_showTabs = showTabs;
}

void KateRenderer::setShowSpaces(KateDocumentConfig::WhitespaceRendering showSpaces)
{
    m_showSpaces = showSpaces;
}

void KateRenderer::setShowNonPrintableSpaces(const bool on)
{
    m_showNonPrintableSpaces = on;
}

void KateRenderer::setTabWidth(int tabWidth)
{
    m_tabWidth = tabWidth;
}

bool KateRenderer::showIndentLines() const
{
    return m_config->showIndentationLines();
}

void KateRenderer::setShowIndentLines(bool showIndentLines)
{
    m_config->setShowIndentationLines(showIndentLines);
}

void KateRenderer::setIndentWidth(int indentWidth)
{
    m_indentWidth = indentWidth;
}

void KateRenderer::setShowSelections(bool showSelections)
{
    m_showSelections = showSelections;
}

void KateRenderer::increaseFontSizes(qreal step)
{
    QFont f(config()->baseFont());
    f.setPointSizeF(f.pointSizeF() + step);
    config()->setFont(f);
}

void KateRenderer::resetFontSizes()
{
    QFont f(KateRendererConfig::global()->baseFont());
    config()->setFont(f);
}

void KateRenderer::decreaseFontSizes(qreal step)
{
    QFont f(config()->baseFont());
    if ((f.pointSizeF() - step) > 0) {
        f.setPointSizeF(f.pointSizeF() - step);
    }
    config()->setFont(f);
}

bool KateRenderer::isPrinterFriendly() const
{
    return m_printerFriendly;
}

void KateRenderer::setPrinterFriendly(bool printerFriendly)
{
    m_printerFriendly = printerFriendly;
    setShowTabs(false);
    setShowSpaces(KateDocumentConfig::None);
    setShowSelections(false);
    setDrawCaret(false);
}

void KateRenderer::paintTextLineBackground(QPainter &paint, KateLineLayoutPtr layout, int currentViewLine, int xStart, int xEnd)
{
    if (isPrinterFriendly()) {
        return;
    }

    // Normal background color
    QColor backgroundColor(config()->backgroundColor());

    // paint the current line background if we're on the current line
    QColor currentLineColor = config()->highlightedLineColor();

    // Check for mark background
    int markRed = 0, markGreen = 0, markBlue = 0, markCount = 0;

    // Retrieve marks for this line
    uint mrk = m_doc->mark(layout->line());
    if (mrk) {
        for (uint bit = 0; bit < 32; bit++) {
            KTextEditor::MarkInterface::MarkTypes markType = (KTextEditor::MarkInterface::MarkTypes)(1 << bit);
            if (mrk & markType) {
                QColor markColor = config()->lineMarkerColor(markType);

                if (markColor.isValid()) {
                    markCount++;
                    markRed += markColor.red();
                    markGreen += markColor.green();
                    markBlue += markColor.blue();
                }
            }
        } // for
    }     // Marks

    if (markCount) {
        markRed /= markCount;
        markGreen /= markCount;
        markBlue /= markCount;
        backgroundColor.setRgb(int((backgroundColor.red() * 0.9) + (markRed * 0.1)), int((backgroundColor.green() * 0.9) + (markGreen * 0.1)), int((backgroundColor.blue() * 0.9) + (markBlue * 0.1)));
    }

    // Draw line background
    paint.fillRect(0, 0, xEnd - xStart, lineHeight() * layout->viewLineCount(), backgroundColor);

    // paint the current line background if we're on the current line
    if (currentViewLine != -1) {
        if (markCount) {
            markRed /= markCount;
            markGreen /= markCount;
            markBlue /= markCount;
            currentLineColor.setRgb(int((currentLineColor.red() * 0.9) + (markRed * 0.1)), int((currentLineColor.green() * 0.9) + (markGreen * 0.1)), int((currentLineColor.blue() * 0.9) + (markBlue * 0.1)));
        }

        paint.fillRect(0, lineHeight() * currentViewLine, xEnd - xStart, lineHeight(), currentLineColor);
    }
}

void KateRenderer::paintTabstop(QPainter &paint, qreal x, qreal y)
{
    QPen penBackup(paint.pen());
    QPen pen(config()->tabMarkerColor());
    pen.setWidthF(qMax(1.0, spaceWidth() / 10.0));
    paint.setPen(pen);

    int dist = spaceWidth() * 0.3;
    QPoint points[8];
    points[0] = QPoint(x - dist, y - dist);
    points[1] = QPoint(x, y);
    points[2] = QPoint(x, y);
    points[3] = QPoint(x - dist, y + dist);
    x += spaceWidth() / 3.0;
    points[4] = QPoint(x - dist, y - dist);
    points[5] = QPoint(x, y);
    points[6] = QPoint(x, y);
    points[7] = QPoint(x - dist, y + dist);
    paint.drawLines(points, 4);
    paint.setPen(penBackup);
}

void KateRenderer::paintSpace(QPainter &paint, qreal x, qreal y)
{
    QPen penBackup(paint.pen());
    QPen pen(config()->tabMarkerColor());

    pen.setWidthF(m_markerSize);
    pen.setCapStyle(Qt::RoundCap);
    paint.setPen(pen);
    paint.setRenderHint(QPainter::Antialiasing, true);
    paint.drawPoint(QPointF(x, y));
    paint.setPen(penBackup);
    paint.setRenderHint(QPainter::Antialiasing, false);
}

void KateRenderer::paintNonBreakSpace(QPainter &paint, qreal x, qreal y)
{
    QPen penBackup(paint.pen());
    QPen pen(config()->tabMarkerColor());
    pen.setWidthF(qMax(1.0, spaceWidth() / 10.0));
    paint.setPen(pen);

    const int height = fontHeight();
    const int width = spaceWidth();

    QPoint points[6];
    points[0] = QPoint(x + width / 10, y + height / 4);
    points[1] = QPoint(x + width / 10, y + height / 3);
    points[2] = QPoint(x + width / 10, y + height / 3);
    points[3] = QPoint(x + width - width / 10, y + height / 3);
    points[4] = QPoint(x + width - width / 10, y + height / 3);
    points[5] = QPoint(x + width - width / 10, y + height / 4);
    paint.drawLines(points, 3);
    paint.setPen(penBackup);
}

void KateRenderer::paintNonPrintableSpaces(QPainter &paint, qreal x, qreal y, const QChar &chr)
{
    paint.save();
    QPen pen(config()->spellingMistakeLineColor());
    pen.setWidthF(qMax(1.0, spaceWidth() * 0.1));
    paint.setPen(pen);

    const int height = fontHeight();
    const int width = m_fontMetrics.boundingRect(chr).width();
    const int offset = spaceWidth() * 0.1;

    QPoint points[8];
    points[0] = QPoint(x - offset, y + offset);
    points[1] = QPoint(x + width + offset, y + offset);
    points[2] = QPoint(x + width + offset, y + offset);
    points[3] = QPoint(x + width + offset, y - height - offset);
    points[4] = QPoint(x + width + offset, y - height - offset);
    points[5] = QPoint(x - offset, y - height - offset);
    points[6] = QPoint(x - offset, y - height - offset);
    points[7] = QPoint(x - offset, y + offset);
    paint.drawLines(points, 4);
    paint.restore();
}

void KateRenderer::paintIndentMarker(QPainter &paint, uint x, uint y /*row*/)
{
    QPen penBackup(paint.pen());
    QPen myPen(config()->indentationLineColor());
    static const QVector<qreal> dashPattern = QVector<qreal>() << 1 << 1;
    myPen.setDashPattern(dashPattern);
    if (y % 2) {
        myPen.setDashOffset(1);
    }
    paint.setPen(myPen);

    const int height = fontHeight();
    const int top = 0;
    const int bottom = height - 1;

    QPainter::RenderHints renderHints = paint.renderHints();
    paint.setRenderHints(renderHints, false);

    paint.drawLine(x + 2, top, x + 2, bottom);

    paint.setRenderHints(renderHints, true);

    paint.setPen(penBackup);
}

static bool rangeLessThanForRenderer(const Kate::TextRange *a, const Kate::TextRange *b)
{
    // compare Z-Depth first
    // smaller Z-Depths should win!
    if (a->zDepth() > b->zDepth()) {
        return true;
    } else if (a->zDepth() < b->zDepth()) {
        return false;
    }

    // end of a > end of b?
    if (a->end().toCursor() > b->end().toCursor()) {
        return true;
    }

    // if ends are equal, start of a < start of b?
    if (a->end().toCursor() == b->end().toCursor()) {
        return a->start().toCursor() < b->start().toCursor();
    }

    return false;
}

QVector<QTextLayout::FormatRange> KateRenderer::decorationsForLine(const Kate::TextLine &textLine, int line, bool selectionsOnly, bool completionHighlight, bool completionSelected) const
{
    // limit number of attributes we can highlight in reasonable time
    const int limitOfRanges = 1024;
    auto rangesWithAttributes = m_doc->buffer().rangesForLine(line, m_printerFriendly ? nullptr : m_view, true);
    if (rangesWithAttributes.size() > limitOfRanges) {
        rangesWithAttributes.clear();
    }

    // Don't compute the highlighting if there isn't going to be any highlighting
    const auto &al = textLine->attributesList();
    if (!(selectionsOnly || !al.isEmpty() || !rangesWithAttributes.isEmpty())) {
        return QVector<QTextLayout::FormatRange>();
    }

    // Add the inbuilt highlighting to the list, limit with limitOfRanges
    RenderRangeVector renderRanges;
    if (!al.isEmpty()) {
        auto &currentRange = renderRanges.pushNewRange();
        for (int i = 0; i < std::min(al.count(), limitOfRanges); ++i) {
            if (al[i].length > 0 && al[i].attributeValue > 0) {
                currentRange.addRange(KTextEditor::Range(KTextEditor::Cursor(line, al[i].offset), al[i].length), specificAttribute(al[i].attributeValue));
            }
        }
    }

    if (!completionHighlight) {
        // check for dynamic hl stuff
        const QSet<Kate::TextRange *> *rangesMouseIn = m_view ? m_view->rangesMouseIn() : nullptr;
        const QSet<Kate::TextRange *> *rangesCaretIn = m_view ? m_view->rangesCaretIn() : nullptr;
        bool anyDynamicHlsActive = m_view && (!rangesMouseIn->empty() || !rangesCaretIn->empty());

        // sort all ranges, we want that the most specific ranges win during rendering, multiple equal ranges are kind of random, still better than old smart rangs behavior ;)
        std::sort(rangesWithAttributes.begin(), rangesWithAttributes.end(), rangeLessThanForRenderer);

        // loop over all ranges
        for (int i = 0; i < rangesWithAttributes.size(); ++i) {
            // real range
            Kate::TextRange *kateRange = rangesWithAttributes[i];

            // calculate attribute, default: normal attribute
            KTextEditor::Attribute::Ptr attribute = kateRange->attribute();
            if (anyDynamicHlsActive) {
                // check mouse in
                if (KTextEditor::Attribute::Ptr attributeMouseIn = attribute->dynamicAttribute(KTextEditor::Attribute::ActivateMouseIn)) {
                    if (rangesMouseIn->contains(kateRange)) {
                        attribute = attributeMouseIn;
                    }
                }

                // check caret in
                if (KTextEditor::Attribute::Ptr attributeCaretIn = attribute->dynamicAttribute(KTextEditor::Attribute::ActivateCaretIn)) {
                    if (rangesCaretIn->contains(kateRange)) {
                        attribute = attributeCaretIn;
                    }
                }
            }

            // span range
            renderRanges.pushNewRange().addRange(*kateRange, attribute);
        }
    }

    // Add selection highlighting if we're creating the selection decorations
    if ((m_view && selectionsOnly && showSelections() && m_view->selection()) || (completionHighlight && completionSelected) || (m_view && m_view->blockSelection())) {
        auto &currentRange = renderRanges.pushNewRange();

        // Set up the selection background attribute TODO: move this elsewhere, eg. into the config?
        static KTextEditor::Attribute::Ptr backgroundAttribute;
        if (!backgroundAttribute) {
            backgroundAttribute = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
        }

        backgroundAttribute->setBackground(config()->selectionColor());
        backgroundAttribute->setForeground(attribute(KTextEditor::dsNormal)->selectedForeground().color());

        // Create a range for the current selection
        if (completionHighlight && completionSelected) {
            currentRange.addRange(KTextEditor::Range(line, 0, line + 1, 0), backgroundAttribute);
        } else if (m_view->blockSelection() && m_view->selectionRange().overlapsLine(line)) {
            currentRange.addRange(m_doc->rangeOnLine(m_view->selectionRange(), line), backgroundAttribute);
        } else {
            currentRange.addRange(m_view->selectionRange(), backgroundAttribute);
        }
    }

    // no render ranges, nothing to do, else we loop below endless!
    if (renderRanges.isEmpty()) {
        return QVector<QTextLayout::FormatRange>();
    }

    // Calculate the range which we need to iterate in order to get the highlighting for just this line
    KTextEditor::Cursor currentPosition, endPosition;
    if (m_view && selectionsOnly) {
        if (m_view->blockSelection()) {
            KTextEditor::Range subRange = m_doc->rangeOnLine(m_view->selectionRange(), line);
            currentPosition = subRange.start();
            endPosition = subRange.end();
        } else {
            KTextEditor::Range rangeNeeded = m_view->selectionRange() & KTextEditor::Range(line, 0, line + 1, 0);

            currentPosition = qMax(KTextEditor::Cursor(line, 0), rangeNeeded.start());
            endPosition = qMin(KTextEditor::Cursor(line + 1, 0), rangeNeeded.end());
        }
    } else {
        currentPosition = KTextEditor::Cursor(line, 0);
        endPosition = KTextEditor::Cursor(line + 1, 0);
    }

    // Main iterative loop.  This walks through each set of highlighting ranges, and stops each
    // time the highlighting changes.  It then creates the corresponding QTextLayout::FormatRanges.
    QVector<QTextLayout::FormatRange> newHighlight;
    while (currentPosition < endPosition) {
        renderRanges.advanceTo(currentPosition);

        if (!renderRanges.hasAttribute()) {
            // No attribute, don't need to create a FormatRange for this text range
            currentPosition = renderRanges.nextBoundary();
            continue;
        }

        KTextEditor::Cursor nextPosition = renderRanges.nextBoundary();

        // Create the format range and populate with the correct start, length and format info
        QTextLayout::FormatRange fr;
        fr.start = currentPosition.column();

        if (nextPosition < endPosition || endPosition.line() <= line) {
            fr.length = nextPosition.column() - currentPosition.column();

        } else {
            // +1 to force background drawing at the end of the line when it's warranted
            fr.length = textLine->length() - currentPosition.column() + 1;
        }

        KTextEditor::Attribute::Ptr a = renderRanges.generateAttribute();
        if (a) {
            fr.format = *a;

            if (selectionsOnly) {
                assignSelectionBrushesFromAttribute(fr, *a);
            }
        }

        newHighlight.append(fr);

        currentPosition = nextPosition;
    }

    return newHighlight;
}

void KateRenderer::assignSelectionBrushesFromAttribute(QTextLayout::FormatRange &target, const KTextEditor::Attribute &attribute) const
{
    if (attribute.hasProperty(SelectedForeground)) {
        target.format.setForeground(attribute.selectedForeground());
    }
    if (attribute.hasProperty(SelectedBackground)) {
        target.format.setBackground(attribute.selectedBackground());
    }
}

void KateRenderer::paintTextLine(QPainter &paint, KateLineLayoutPtr range, int xStart, int xEnd, const KTextEditor::Cursor *cursor, PaintTextLineFlags flags)
{
    Q_ASSERT(range->isValid());

    //   qCDebug(LOG_KTE)<<"KateRenderer::paintTextLine";

    // font data
    const QFontMetricsF &fm = m_fontMetrics;

    int currentViewLine = -1;
    if (cursor && cursor->line() == range->line()) {
        currentViewLine = range->viewLineForColumn(cursor->column());
    }

    paintTextLineBackground(paint, range, currentViewLine, xStart, xEnd);

    // Draws the dashed underline at the start of a folded block of text.
    if (!(flags & SkipDrawFirstInvisibleLineUnderlined) && range->startsInvisibleBlock()) {
        QPen pen(config()->foldingColor());
        pen.setCosmetic(true);
        pen.setStyle(Qt::DashLine);
        pen.setDashOffset(xStart);
        pen.setWidth(2);
        paint.setPen(pen);
        paint.drawLine(0, (lineHeight() * range->viewLineCount()) - 2, xEnd - xStart, (lineHeight() * range->viewLineCount()) - 2);
    }

    if (range->layout()) {
        bool drawSelection = m_view && m_view->selection() && showSelections() && m_view->selectionRange().overlapsLine(range->line());
        // Draw selection in block selection mode. We need 2 kinds of selections that QTextLayout::draw can't render:
        //   - past-end-of-line selection and
        //   - 0-column-wide selection (used to indicate where text will be typed)
        if (drawSelection && m_view->blockSelection()) {
            int selectionStartColumn = m_doc->fromVirtualColumn(range->line(), m_doc->toVirtualColumn(m_view->selectionRange().start()));
            int selectionEndColumn = m_doc->fromVirtualColumn(range->line(), m_doc->toVirtualColumn(m_view->selectionRange().end()));
            QBrush selectionBrush = config()->selectionColor();
            if (selectionStartColumn != selectionEndColumn) {
                KateTextLayout lastLine = range->viewLine(range->viewLineCount() - 1);
                if (selectionEndColumn > lastLine.startCol()) {
                    int selectionStartX = (selectionStartColumn > lastLine.startCol()) ? cursorToX(lastLine, selectionStartColumn, true) : 0;
                    int selectionEndX = cursorToX(lastLine, selectionEndColumn, true);
                    paint.fillRect(QRect(selectionStartX - xStart, (int)lastLine.lineLayout().y(), selectionEndX - selectionStartX, lineHeight()), selectionBrush);
                }
            } else {
                const int selectStickWidth = 2;
                KateTextLayout selectionLine = range->viewLine(range->viewLineForColumn(selectionStartColumn));
                int selectionX = cursorToX(selectionLine, selectionStartColumn, true);
                paint.fillRect(QRect(selectionX - xStart, (int)selectionLine.lineLayout().y(), selectStickWidth, lineHeight()), selectionBrush);
            }
        }

        QVector<QTextLayout::FormatRange> additionalFormats;
        if (range->length() > 0) {
            // We may have changed the pen, be absolutely sure it gets set back to
            // normal foreground color before drawing text for text that does not
            // set the pen color
            paint.setPen(attribute(KTextEditor::dsNormal)->foreground().color());
            // Draw the text :)
            if (drawSelection) {
                additionalFormats = decorationsForLine(range->textLine(), range->line(), true);
                range->layout()->draw(&paint, QPoint(-xStart, 0), additionalFormats);

            } else {
                range->layout()->draw(&paint, QPoint(-xStart, 0));
            }
        }

        QBrush backgroundBrush;
        bool backgroundBrushSet = false;

        // Loop each individual line for additional text decoration etc.
        QVectorIterator<QTextLayout::FormatRange> it = range->layout()->formats();
        QVectorIterator<QTextLayout::FormatRange> it2 = additionalFormats;
        for (int i = 0; i < range->viewLineCount(); ++i) {
            KateTextLayout line = range->viewLine(i);

            bool haveBackground = false;
            // Determine the background to use, if any, for the end of this view line
            backgroundBrushSet = false;
            while (it2.hasNext()) {
                const QTextLayout::FormatRange &fr = it2.peekNext();
                if (fr.start > line.endCol()) {
                    break;
                }

                if (fr.start + fr.length > line.endCol()) {
                    if (fr.format.hasProperty(QTextFormat::BackgroundBrush)) {
                        backgroundBrushSet = true;
                        backgroundBrush = fr.format.background();
                    }

                    haveBackground = true;
                    break;
                }

                it2.next();
            }

            while (!haveBackground && it.hasNext()) {
                const QTextLayout::FormatRange &fr = it.peekNext();
                if (fr.start > line.endCol()) {
                    break;
                }

                if (fr.start + fr.length > line.endCol()) {
                    if (fr.format.hasProperty(QTextFormat::BackgroundBrush)) {
                        backgroundBrushSet = true;
                        backgroundBrush = fr.format.background();
                    }

                    break;
                }

                it.next();
            }

            // Draw selection or background color outside of areas where text is rendered
            if (!m_printerFriendly) {
                bool draw = false;
                QBrush drawBrush;
                if (m_view && m_view->selection() && !m_view->blockSelection() && m_view->lineEndSelected(line.end(true))) {
                    draw = true;
                    drawBrush = config()->selectionColor();
                } else if (backgroundBrushSet && !m_view->blockSelection()) {
                    draw = true;
                    drawBrush = backgroundBrush;
                }

                if (draw) {
                    int fillStartX = line.endX() - line.startX() + line.xOffset() - xStart;
                    int fillStartY = lineHeight() * i;
                    int width = xEnd - xStart - fillStartX;
                    int height = lineHeight();

                    // reverse X for right-aligned lines
                    if (range->layout()->textOption().alignment() == Qt::AlignRight) {
                        fillStartX = 0;
                    }

                    if (width > 0) {
                        QRect area(fillStartX, fillStartY, width, height);
                        paint.fillRect(area, drawBrush);
                    }
                }

                // Draw indent lines
                if (showIndentLines() && i == 0) {
                    const qreal w = spaceWidth();
                    const int lastIndentColumn = range->textLine()->indentDepth(m_tabWidth);

                    for (int x = m_indentWidth; x < lastIndentColumn; x += m_indentWidth) {
                        paintIndentMarker(paint, x * w + 1 - xStart, range->line());
                    }
                }
            }

            // draw an open box to mark non-breaking spaces
            const QString &text = range->textLine()->string();
            int y = lineHeight() * i + fm.ascent() - fm.strikeOutPos();
            int nbSpaceIndex = text.indexOf(nbSpaceChar, line.lineLayout().xToCursor(xStart));

            while (nbSpaceIndex != -1 && nbSpaceIndex < line.endCol()) {
                int x = line.lineLayout().cursorToX(nbSpaceIndex);
                if (x > xEnd) {
                    break;
                }
                paintNonBreakSpace(paint, x - xStart, y);
                nbSpaceIndex = text.indexOf(nbSpaceChar, nbSpaceIndex + 1);
            }

            // draw tab stop indicators
            if (showTabs()) {
                int tabIndex = text.indexOf(tabChar, line.lineLayout().xToCursor(xStart));
                while (tabIndex != -1 && tabIndex < line.endCol()) {
                    int x = line.lineLayout().cursorToX(tabIndex);
                    if (x > xEnd) {
                        break;
                    }
                    paintTabstop(paint, x - xStart + spaceWidth() / 2.0, y);
                    tabIndex = text.indexOf(tabChar, tabIndex + 1);
                }
            }

            // draw trailing spaces
            if (showSpaces() != KateDocumentConfig::None) {
                int spaceIndex = line.endCol() - 1;
                const int trailingPos = showSpaces() == KateDocumentConfig::All ? 0 : qMax(range->textLine()->lastChar(), 0);

                if (spaceIndex >= trailingPos) {
                    for (; spaceIndex >= line.startCol(); --spaceIndex) {
                        if (!text.at(spaceIndex).isSpace()) {
                            if (showSpaces() == KateDocumentConfig::Trailing)
                                break;
                            else
                                continue;
                        }

                        if (text.at(spaceIndex) != QLatin1Char('\t') || !showTabs()) {
                            if (range->layout()->textOption().alignment() == Qt::AlignRight) { // Draw on left for RTL lines
                                paintSpace(paint, line.lineLayout().cursorToX(spaceIndex) - xStart - spaceWidth() / 2.0, y);
                            } else {
                                paintSpace(paint, line.lineLayout().cursorToX(spaceIndex) - xStart + spaceWidth() / 2.0, y);
                            }
                        }
                    }
                }
            }

            if (showNonPrintableSpaces()) {
                const int y = lineHeight() * i + fm.ascent();

                static const QRegularExpression nonPrintableSpacesRegExp(QStringLiteral("[\\x{2000}-\\x{200F}\\x{2028}-\\x{202F}\\x{205F}-\\x{2064}\\x{206A}-\\x{206F}]"));
                QRegularExpressionMatchIterator i = nonPrintableSpacesRegExp.globalMatch(text, line.lineLayout().xToCursor(xStart));

                while (i.hasNext()) {
                    const int charIndex = i.next().capturedStart();

                    const int x = line.lineLayout().cursorToX(charIndex);
                    if (x > xEnd) {
                        break;
                    }

                    paintNonPrintableSpaces(paint, x - xStart, y, text[charIndex]);
                }
            }
        }

        // draw word-wrap-honor-indent filling
        if ((range->viewLineCount() > 1) && range->shiftX() && (range->shiftX() > xStart)) {
            if (backgroundBrushSet)
                paint.fillRect(0, lineHeight(), range->shiftX() - xStart, lineHeight() * (range->viewLineCount() - 1), backgroundBrush);
            paint.fillRect(0, lineHeight(), range->shiftX() - xStart, lineHeight() * (range->viewLineCount() - 1), QBrush(config()->wordWrapMarkerColor(), Qt::Dense4Pattern));
        }

        // Draw caret
        if (drawCaret() && cursor && range->includesCursor(*cursor)) {
            int caretWidth, lineWidth = 2;
            QColor color;
            QTextLine line = range->layout()->lineForTextPosition(qMin(cursor->column(), range->length()));

            // Determine the caret's style
            caretStyles style = caretStyle();

            // Make the caret the desired width
            if (style == Line) {
                caretWidth = lineWidth;
            } else if (line.isValid() && cursor->column() < range->length()) {
                caretWidth = int(line.cursorToX(cursor->column() + 1) - line.cursorToX(cursor->column()));
                if (caretWidth < 0) {
                    caretWidth = -caretWidth;
                }
            } else {
                caretWidth = spaceWidth();
            }

            // Determine the color
            if (m_caretOverrideColor.isValid()) {
                // Could actually use the real highlighting system for this...
                // would be slower, but more accurate for corner cases
                color = m_caretOverrideColor;
            } else {
                // search for the FormatRange that includes the cursor
                const auto formatRanges = range->layout()->formats();
                for (const QTextLayout::FormatRange &r : formatRanges) {
                    if ((r.start <= cursor->column()) && ((r.start + r.length) > cursor->column())) {
                        // check for Qt::NoBrush, as the returned color is black() and no invalid QColor
                        QBrush foregroundBrush = r.format.foreground();
                        if (foregroundBrush != Qt::NoBrush) {
                            color = r.format.foreground().color();
                        }
                        break;
                    }
                }
                // still no color found, fall back to default style
                if (!color.isValid()) {
                    color = attribute(KTextEditor::dsNormal)->foreground().color();
                }
            }

            paint.save();
            switch (style) {
            case Line:
                paint.setPen(QPen(color, caretWidth));
                break;
            case Block:
                // use a gray caret so it's possible to see the character
                color.setAlpha(128);
                paint.setPen(QPen(color, caretWidth));
                break;
            case Underline:
                break;
            case Half:
                color.setAlpha(128);
                paint.setPen(QPen(color, caretWidth));
                break;
            }

            if (cursor->column() <= range->length()) {
                range->layout()->drawCursor(&paint, QPoint(-xStart, 0), cursor->column(), caretWidth);
            } else {
                // Off the end of the line... must be block mode. Draw the caret ourselves.
                const KateTextLayout &lastLine = range->viewLine(range->viewLineCount() - 1);
                int x = cursorToX(lastLine, KTextEditor::Cursor(range->line(), cursor->column()), true);
                if ((x >= xStart) && (x <= xEnd)) {
                    paint.fillRect(x - xStart, (int)lastLine.lineLayout().y(), caretWidth, lineHeight(), color);
                }
            }

            paint.restore();
        }
    }

    // show word wrap marker if desirable
    if ((!isPrinterFriendly()) && config()->wordWrapMarker()) {
        const QPainter::RenderHints backupRenderHints = paint.renderHints();
        paint.setPen(config()->wordWrapMarkerColor());
        int _x = qreal(m_doc->config()->wordWrapAt()) * fm.horizontalAdvance(QLatin1Char('x')) - xStart;
        paint.drawLine(_x, 0, _x, lineHeight());
        paint.setRenderHints(backupRenderHints);
    }

    // Draw inline notes
    if (!isPrinterFriendly()) {
        const auto inlineNotes = m_view->inlineNotes(range->line());
        for (const auto &inlineNoteData : inlineNotes) {
            KTextEditor::InlineNote inlineNote(inlineNoteData);
            const int column = inlineNote.position().column();
            int viewLine = range->viewLineForColumn(column);

            // Determine the position where to paint the note.
            // We start by getting the x coordinate of cursor placed to the column.
            qreal x = range->viewLine(viewLine).lineLayout().cursorToX(column) - xStart;
            int textLength = range->length();
            if (column == 0 || column < textLength) {
                // If the note is inside text or at the beginning, then there is a hole in the text where the
                // note should be painted and the cursor gets placed at the right side of it. So we have to
                // subtract the width of the note to get to left side of the hole.
                x -= inlineNote.width();
            } else {
                // If the note is outside the text, then the X coordinate is located at the end of the line.
                // Add appropriate amount of spaces to reach the required column.
                x += spaceWidth() * (column - textLength);
            }

            qreal y = lineHeight() * viewLine;

            // Paint the note
            paint.save();
            paint.translate(x, y);
            inlineNote.provider()->paintInlineNote(inlineNote, paint);
            paint.restore();
        }
    }
}

uint KateRenderer::fontHeight() const
{
    return m_fontHeight;
}

uint KateRenderer::documentHeight() const
{
    return m_doc->lines() * lineHeight();
}

int KateRenderer::lineHeight() const
{
    return fontHeight();
}

bool KateRenderer::getSelectionBounds(int line, int lineLength, int &start, int &end) const
{
    bool hasSel = false;

    if (m_view->selection() && !m_view->blockSelection()) {
        if (m_view->lineIsSelection(line)) {
            start = m_view->selectionRange().start().column();
            end = m_view->selectionRange().end().column();
            hasSel = true;
        } else if (line == m_view->selectionRange().start().line()) {
            start = m_view->selectionRange().start().column();
            end = lineLength;
            hasSel = true;
        } else if (m_view->selectionRange().containsLine(line)) {
            start = 0;
            end = lineLength;
            hasSel = true;
        } else if (line == m_view->selectionRange().end().line()) {
            start = 0;
            end = m_view->selectionRange().end().column();
            hasSel = true;
        }
    } else if (m_view->lineHasSelected(line)) {
        start = m_view->selectionRange().start().column();
        end = m_view->selectionRange().end().column();
        hasSel = true;
    }

    if (start > end) {
        int temp = end;
        end = start;
        start = temp;
    }

    return hasSel;
}

void KateRenderer::updateConfig()
{
    // update the attribute list pointer
    updateAttributes();

    // update font height, do this before we update the view!
    updateFontHeight();

    // trigger view update, if any!
    if (m_view) {
        m_view->updateRendererConfig();
    }
}

void KateRenderer::updateFontHeight()
{
    /**
     * cache font + metrics
     */
    m_font = config()->baseFont();
    m_fontMetrics = QFontMetricsF(m_font);

    /**
     * ensure minimal height of one pixel to not fall in the div by 0 trap somewhere
     *
     * use a line spacing that matches the code in qt to layout/paint text
     *
     * see bug 403868
     * https://github.com/qt/qtbase/blob/5.12/src/gui/text/qtextlayout.cpp (line 2270 at the moment) where the text height is set as:
     *
     * qreal height = maxY + fontHeight - minY;
     *
     * with fontHeight:
     *
     * qreal fontHeight = font.ascent() + font.descent();
     */
    m_fontHeight = qMax(1, qCeil(m_fontMetrics.ascent() + m_fontMetrics.descent()));
    m_fontAscent = m_fontMetrics.ascent();
}

void KateRenderer::updateMarkerSize()
{
    // marker size will be calculated over the value defined
    // on dialog

    m_markerSize = spaceWidth() / (3.5 - (m_doc->config()->markerSize() * 0.5));
}

qreal KateRenderer::spaceWidth() const
{
    return m_fontMetrics.horizontalAdvance(spaceChar);
}

void KateRenderer::layoutLine(KateLineLayoutPtr lineLayout, int maxwidth, bool cacheLayout) const
{
    // if maxwidth == -1 we have no wrap

    Kate::TextLine textLine = lineLayout->textLine();
    Q_ASSERT(textLine);

    QTextLayout *l = lineLayout->layout();
    if (!l) {
        l = new QTextLayout(textLine->string(), m_font);
    } else {
        l->setText(textLine->string());
        l->setFont(m_font);
    }

    l->setCacheEnabled(cacheLayout);

    // Initial setup of the QTextLayout.

    // Tab width
    QTextOption opt;
    opt.setFlags(QTextOption::IncludeTrailingSpaces);
    opt.setTabStopDistance(m_tabWidth * m_fontMetrics.horizontalAdvance(spaceChar));
    if (m_view->config()->dynWrapAnywhere()) {
        opt.setWrapMode(QTextOption::WrapAnywhere);
    } else {
        opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    }

    // Find the first strong character in the string.
    // If it is an RTL character, set the base layout direction of the string to RTL.
    //
    // See https://www.unicode.org/reports/tr9/#The_Paragraph_Level (Sections P2 & P3).
    // Qt's text renderer ("scribe") version 4.2 assumes a "higher-level protocol"
    // (such as KatePart) will specify the paragraph level, so it does not apply P2 & P3
    // by itself. If this ever change in Qt, the next code block could be removed.
    if (isLineRightToLeft(lineLayout)) {
        opt.setAlignment(Qt::AlignRight);
        opt.setTextDirection(Qt::RightToLeft);
    } else {
        opt.setAlignment(Qt::AlignLeft);
        opt.setTextDirection(Qt::LeftToRight);
    }

    l->setTextOption(opt);

    // Syntax highlighting, inbuilt and arbitrary
    QVector<QTextLayout::FormatRange> decorations = decorationsForLine(textLine, lineLayout->line());

    int firstLineOffset = 0;

    if (!isPrinterFriendly()) {
        const auto inlineNotes = m_view->inlineNotes(lineLayout->line());
        for (const KTextEditor::InlineNote &inlineNote : inlineNotes) {
            const int column = inlineNote.position().column();
            int width = inlineNote.width();

            // Make space for every inline note.
            // If it is on column 0 (at the beginning of the line), we must offset the first line.
            // If it is inside the text, we use absolute letter spacing to create space for it between the two letters.
            // If it is outside of the text, we don't have to make space for it.
            if (column == 0) {
                firstLineOffset = width;
            } else if (column < l->text().length()) {
                QTextCharFormat text_char_format;
                text_char_format.setFontLetterSpacing(width);
                text_char_format.setFontLetterSpacingType(QFont::AbsoluteSpacing);
                decorations.append(QTextLayout::FormatRange {column - 1, 1, text_char_format});
            }
        }
    }
    l->setFormats(decorations);

    // Begin layouting
    l->beginLayout();

    int height = 0;
    int shiftX = 0;

    bool needShiftX = (maxwidth != -1) && m_view && (m_view->config()->dynWordWrapAlignIndent() > 0);

    forever {
        QTextLine line = l->createLine();
        if (!line.isValid()) {
            break;
        }

        if (maxwidth > 0) {
            line.setLineWidth(maxwidth);
        } else {
            line.setLineWidth(INT_MAX);
        }

        // we include the leading, this must match the ::updateFontHeight code!
        line.setLeadingIncluded(true);

        line.setPosition(QPoint(line.lineNumber() ? shiftX : firstLineOffset, height - line.ascent() + m_fontAscent));

        if (needShiftX && line.width() > 0) {
            needShiftX = false;
            // Determine x offset for subsequent-lines-of-paragraph indenting
            int pos = textLine->nextNonSpaceChar(0);

            if (pos > 0) {
                shiftX = (int)line.cursorToX(pos);
            }

            // check for too deep shift value and limit if necessary
            if (shiftX > ((double)maxwidth / 100 * m_view->config()->dynWordWrapAlignIndent())) {
                shiftX = 0;
            }

            // if shiftX > 0, the maxwidth has to adapted
            maxwidth -= shiftX;

            lineLayout->setShiftX(shiftX);
        }

        height += lineHeight();
    }

    l->endLayout();

    lineLayout->setLayout(l);
}

// 1) QString::isRightToLeft() sux
// 2) QString::isRightToLeft() is marked as internal (WTF?)
// 3) QString::isRightToLeft() does not seem to work on my setup
// 4) isStringRightToLeft() should behave much better than QString::isRightToLeft() therefore:
// 5) isStringRightToLeft() kicks ass
bool KateRenderer::isLineRightToLeft(KateLineLayoutPtr lineLayout) const
{
    QString s = lineLayout->textLine()->string();
    int i = 0;

    // borrowed from QString::updateProperties()
    while (i != s.length()) {
        QChar c = s.at(i);

        switch (c.direction()) {
        case QChar::DirL:
        case QChar::DirLRO:
        case QChar::DirLRE:
            return false;

        case QChar::DirR:
        case QChar::DirAL:
        case QChar::DirRLO:
        case QChar::DirRLE:
            return true;

        default:
            break;
        }
        i++;
    }

    return false;
#if 0
    // or should we use the direction of the widget?
    QWidget *display = qobject_cast<QWidget *>(view()->parent());
    if (!display) {
        return false;
    }
    return display->layoutDirection() == Qt::RightToLeft;
#endif
}

int KateRenderer::cursorToX(const KateTextLayout &range, int col, bool returnPastLine) const
{
    return cursorToX(range, KTextEditor::Cursor(range.line(), col), returnPastLine);
}

int KateRenderer::cursorToX(const KateTextLayout &range, const KTextEditor::Cursor &pos, bool returnPastLine) const
{
    Q_ASSERT(range.isValid());

    int x;
    if (range.lineLayout().width() > 0) {
        x = (int)range.lineLayout().cursorToX(pos.column());
    } else {
        x = 0;
    }

    int over = pos.column() - range.endCol();
    if (returnPastLine && over > 0) {
        x += over * spaceWidth();
    }

    return x;
}

KTextEditor::Cursor KateRenderer::xToCursor(const KateTextLayout &range, int x, bool returnPastLine) const
{
    Q_ASSERT(range.isValid());
    KTextEditor::Cursor ret(range.line(), range.lineLayout().xToCursor(x));

    // TODO wrong for RTL lines?
    if (returnPastLine && range.endCol(true) == -1 && x > range.width() + range.xOffset()) {
        ret.setColumn(ret.column() + ((x - (range.width() + range.xOffset())) / spaceWidth()));
    }

    return ret;
}

void KateRenderer::setCaretOverrideColor(const QColor &color)
{
    m_caretOverrideColor = color;
}

/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2009 Milian Wolff <mail@milianw.de>

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

#ifndef ABSTRACTEXPORTER_H
#define ABSTRACTEXPORTER_H

#include <QTextStream>

#include <ktexteditor/attribute.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/document.h>
#include <ktexteditor/range.h>
#include <ktexteditor/view.h>

class AbstractExporter
{
public:
    /// If \p m_encapsulate is set, you should add some kind of header in the ctor
    /// to \p m_output.
    AbstractExporter(KTextEditor::View *view, QTextStream &output, const bool encapsulate = false)
        : m_view(view)
        , m_output(output)
        , m_encapsulate(encapsulate)
        , m_defaultAttribute(nullptr)
    {
        QColor defaultBackground;
        if (KTextEditor::ConfigInterface *ciface = qobject_cast<KTextEditor::ConfigInterface *>(m_view)) {
            QVariant variant = ciface->configValue(QStringLiteral("background-color"));
            if (variant.canConvert<QColor>()) {
                defaultBackground = variant.value<QColor>();
            }
        }

        m_defaultAttribute = view->defaultStyleAttribute(KTextEditor::dsNormal);
        m_defaultAttribute->setBackground(QBrush(defaultBackground));
    }

    /// Gets called after everything got exported.
    /// Hence, if \p m_encapsulate is set, you should probably add some kind of footer here.
    virtual ~AbstractExporter()
    {
    }

    /// Begin a new line.
    virtual void openLine() = 0;

    /// Finish the current line.
    virtual void closeLine(const bool lastLine) = 0;

    /// Export \p text with given text attribute \p attrib.
    virtual void exportText(const QString &text, const KTextEditor::Attribute::Ptr &attrib) = 0;

protected:
    KTextEditor::View *m_view;
    QTextStream &m_output;
    bool m_encapsulate;
    KTextEditor::Attribute::Ptr m_defaultAttribute;
};

#endif

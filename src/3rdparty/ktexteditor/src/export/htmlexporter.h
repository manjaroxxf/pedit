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

#ifndef HTMLEXPORTER_H
#define HTMLEXPORTER_H

#include "abstractexporter.h"

/// TODO: add abstract interface for future exporters
class HTMLExporter : public AbstractExporter
{
public:
    HTMLExporter(KTextEditor::View *view, QTextStream &output, const bool withHeaderFooter = false);
    ~HTMLExporter() override;

    void openLine() override;
    void closeLine(const bool lastLine) override;
    void exportText(const QString &text, const KTextEditor::Attribute::Ptr &attrib) override;
};

#endif

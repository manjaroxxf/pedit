/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2007, 2008 Matthew Woehlke <mw_triad@users.sourceforge.net>
    Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
    Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

// BEGIN Includes
#include "kateschema.h"

#include "kateconfig.h"
#include "kateglobal.h"
#include "katepartdebug.h"
#include "katerenderer.h"
#include "kateview.h"

#include <KConfigGroup>
// END

// BEGIN KateSchemaManager
KateSchemaManager::KateSchemaManager()
    : m_config(KTextEditor::EditorPrivate::unitTestMode() ? QString() : QStringLiteral("kateschemarc"), KTextEditor::EditorPrivate::unitTestMode() ? KConfig::SimpleConfig : KConfig::NoGlobals) // skip config for unit tests!
{
}

KConfigGroup KateSchemaManager::schema(const QString &name)
{
    return m_config.group(name);
}

KateSchema KateSchemaManager::schemaData(const QString &name)
{
    KConfigGroup cg(schema(name));
    KateSchema schema;
    schema.rawName = name;
    schema.shippedDefaultSchema = cg.readEntry("ShippedDefaultSchema", 0);
    return schema;
}

static bool schemasCompare(const KateSchema &s1, const KateSchema &s2)
{
    if (s1.shippedDefaultSchema > s2.shippedDefaultSchema) {
        return true;
    }

    return s1.translatedName().localeAwareCompare(s1.translatedName()) < 0;
}

QList<KateSchema> KateSchemaManager::list()
{
    QList<KateSchema> schemas;
    const auto names = m_config.groupList();
    for (const QString &s : names) {
        schemas.append(schemaData(s));
    }

    // sort: prio given by default schema and name
    std::sort(schemas.begin(), schemas.end(), schemasCompare);

    return schemas;
}
// END

// BEGIN SCHEMA ACTION -- the 'View->Schema' menu action
void KateViewSchemaAction::init()
{
    m_group = nullptr;
    m_view = nullptr;
    last = 0;

    connect(menu(), SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()));
}

void KateViewSchemaAction::updateMenu(KTextEditor::ViewPrivate *view)
{
    m_view = view;
}

void KateViewSchemaAction::slotAboutToShow()
{
    KTextEditor::ViewPrivate *view = m_view;

    QList<KateSchema> schemas = KTextEditor::EditorPrivate::self()->schemaManager()->list();

    if (!m_group) {
        m_group = new QActionGroup(menu());
        m_group->setExclusive(true);
    }

    for (int z = 0; z < schemas.count(); z++) {
        QString hlName = schemas[z].translatedName();

        if (!names.contains(hlName)) {
            names << hlName;
            QAction *a = menu()->addAction(hlName, this, SLOT(setSchema()));
            a->setData(schemas[z].rawName);
            a->setCheckable(true);
            a->setActionGroup(m_group);
        }
    }

    if (!view) {
        return;
    }

    QString id = view->renderer()->config()->schema();
    const auto menuActions = menu()->actions();
    for (QAction *a : menuActions) {
        a->setChecked(a->data().toString() == id);
    }
}

void KateViewSchemaAction::setSchema()
{
    QAction *action = qobject_cast<QAction *>(sender());

    if (!action) {
        return;
    }
    QString mode = action->data().toString();

    KTextEditor::ViewPrivate *view = m_view;

    if (view) {
        view->renderer()->config()->setSchema(mode);
    }
}
// END SCHEMA ACTION

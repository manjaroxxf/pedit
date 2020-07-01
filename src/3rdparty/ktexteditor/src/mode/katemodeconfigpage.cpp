/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>

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
#include "katemodeconfigpage.h"

#include "kateautoindent.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katesyntaxmanager.h"
#include "kateview.h"

#include "ui_filetypeconfigwidget.h"

#include "katepartdebug.h"
#include <KMimeTypeChooser>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QToolButton>
// END Includes

ModeConfigPage::ModeConfigPage(QWidget *parent)
    : KateConfigPage(parent)
{
    m_lastType = -1;

    // This will let us have more separation between this page and
    // the QTabWidget edge (ereslibre)
    QVBoxLayout *layout = new QVBoxLayout;
    QWidget *newWidget = new QWidget(this);

    ui = new Ui::FileTypeConfigWidget();
    ui->setupUi(newWidget);

    ui->cmbHl->addItem(i18n("<Unchanged>"), QVariant(QString()));
    for (const auto &hl : KateHlManager::self()->modeList()) {
        const auto section = hl.translatedSection();
        if (!section.isEmpty())
            ui->cmbHl->addItem(section + QLatin1Char('/') + hl.translatedName(), QVariant(hl.name()));
        else {
            ui->cmbHl->addItem(hl.translatedName(), QVariant(hl.name()));
        }
    }

    QStringList indentationModes;
    indentationModes << i18n("Use Default");
    indentationModes << KateAutoIndent::listModes();
    ui->cmbIndenter->addItems(indentationModes);

    connect(ui->cmbFiletypes, SIGNAL(activated(int)), this, SLOT(typeChanged(int)));
    connect(ui->btnNew, SIGNAL(clicked()), this, SLOT(newType()));
    connect(ui->btnDelete, SIGNAL(clicked()), this, SLOT(deleteType()));
    ui->btnMimeTypes->setIcon(QIcon::fromTheme(QStringLiteral("tools-wizard")));
    connect(ui->btnMimeTypes, SIGNAL(clicked()), this, SLOT(showMTDlg()));

    reload();

    connect(ui->edtName, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(ui->edtSection, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(ui->edtVariables, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(ui->edtFileExtensions, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(ui->edtMimeTypes, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(ui->sbPriority, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
    connect(ui->cmbHl, SIGNAL(activated(int)), this, SLOT(slotChanged()));
    connect(ui->cmbIndenter, SIGNAL(activated(int)), this, SLOT(slotChanged()));

    // make the context help a bit easier to access
    ui->sbPriority->setToolTip(ui->sbPriority->whatsThis());

    layout->addWidget(newWidget);
    setLayout(layout);
}

ModeConfigPage::~ModeConfigPage()
{
    qDeleteAll(m_types);
    delete ui;
}

void ModeConfigPage::apply()
{
    if (!hasChanged()) {
        return;
    }

    save();

    KTextEditor::EditorPrivate::self()->modeManager()->save(m_types);
}

void ModeConfigPage::reload()
{
    qDeleteAll(m_types);
    m_types.clear();

    // deep copy...
    const QList<KateFileType *> &modeList = KTextEditor::EditorPrivate::self()->modeManager()->list();
    m_types.reserve(modeList.size());
    for (KateFileType *type : modeList) {
        KateFileType *t = new KateFileType();
        *t = *type;
        m_types.append(t);
    }

    update();
}

void ModeConfigPage::reset()
{
    reload();
}

void ModeConfigPage::defaults()
{
    reload();
}

void ModeConfigPage::update()
{
    m_lastType = -1;

    ui->cmbFiletypes->clear();

    for (KateFileType *type : qAsConst(m_types)) {
        if (!type->sectionTranslated().isEmpty()) {
            ui->cmbFiletypes->addItem(type->sectionTranslated() + QLatin1Char('/') + type->nameTranslated());
        } else {
            ui->cmbFiletypes->addItem(type->nameTranslated());
        }
    }

    // get current filetype from active view via the host application
    int currentIndex = 0;
    KTextEditor::ViewPrivate *kv = qobject_cast<KTextEditor::ViewPrivate *>(KTextEditor::EditorPrivate::self()->application()->activeMainWindow()->activeView());
    if (kv) {
        const QString filetypeName = kv->doc()->fileType();
        for (int i = 0; i < m_types.size(); ++i) {
            if (filetypeName == m_types[i]->name) {
                currentIndex = i;
                break;
            }
        }
    }
    ui->cmbFiletypes->setCurrentIndex(currentIndex);
    typeChanged(currentIndex);

    ui->cmbFiletypes->setEnabled(ui->cmbFiletypes->count() > 0);
}

void ModeConfigPage::deleteType()
{
    int type = ui->cmbFiletypes->currentIndex();

    if (type > -1 && type < m_types.count()) {
        delete m_types[type];
        m_types.removeAt(type);
        update();
    }
}

void ModeConfigPage::newType()
{
    QString newN = i18n("New Filetype");

    for (int i = 0; i < m_types.count(); ++i) {
        KateFileType *type = m_types.at(i);
        if (type->name == newN) {
            ui->cmbFiletypes->setCurrentIndex(i);
            typeChanged(i);
            return;
        }
    }

    KateFileType *newT = new KateFileType();
    newT->priority = 0;
    newT->name = newN;
    newT->hlGenerated = false;

    m_types.prepend(newT);

    update();
}

void ModeConfigPage::save()
{
    if (m_lastType != -1) {
        if (!m_types[m_lastType]->hlGenerated) {
            m_types[m_lastType]->name = ui->edtName->text();
            m_types[m_lastType]->section = ui->edtSection->text();
        }
        m_types[m_lastType]->varLine = ui->edtVariables->text();
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        m_types[m_lastType]->wildcards = ui->edtFileExtensions->text().split(QLatin1Char(';'), QString::SkipEmptyParts);
#else
        m_types[m_lastType]->wildcards = ui->edtFileExtensions->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
#endif
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        m_types[m_lastType]->mimetypes = ui->edtMimeTypes->text().split(QLatin1Char(';'), QString::SkipEmptyParts);
#else
        m_types[m_lastType]->mimetypes = ui->edtMimeTypes->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
#endif
        m_types[m_lastType]->priority = ui->sbPriority->value();
        m_types[m_lastType]->hl = ui->cmbHl->itemData(ui->cmbHl->currentIndex()).toString();

        if (ui->cmbIndenter->currentIndex() > 0) {
            m_types[m_lastType]->indenter = KateAutoIndent::modeName(ui->cmbIndenter->currentIndex() - 1);
        } else {
            m_types[m_lastType]->indenter = QString();
        }
    }
}

void ModeConfigPage::typeChanged(int type)
{
    save();

    ui->cmbHl->setEnabled(true);
    ui->btnDelete->setEnabled(true);
    ui->edtName->setEnabled(true);
    ui->edtSection->setEnabled(true);

    if (type > -1 && type < m_types.count()) {
        KateFileType *t = m_types.at(type);

        ui->gbProperties->setTitle(i18n("Properties of %1", ui->cmbFiletypes->currentText()));

        ui->gbProperties->setEnabled(true);
        ui->btnDelete->setEnabled(true);

        ui->edtName->setText(t->nameTranslated());
        ui->edtSection->setText(t->sectionTranslated());
        ui->edtVariables->setText(t->varLine);
        ui->edtFileExtensions->setText(t->wildcards.join(QLatin1Char(';')));
        ui->edtMimeTypes->setText(t->mimetypes.join(QLatin1Char(';')));
        ui->sbPriority->setValue(t->priority);

        ui->cmbHl->setEnabled(!t->hlGenerated);
        ui->btnDelete->setEnabled(!t->hlGenerated);
        ui->edtName->setEnabled(!t->hlGenerated);
        ui->edtSection->setEnabled(!t->hlGenerated);

        // activate current hl...
        for (int i = 0; i < ui->cmbHl->count(); ++i)
            if (ui->cmbHl->itemData(i).toString() == t->hl) {
                ui->cmbHl->setCurrentIndex(i);
            }

        // activate the right indenter
        int indenterIndex = 0;
        if (!t->indenter.isEmpty()) {
            indenterIndex = KateAutoIndent::modeNumber(t->indenter) + 1;
        }
        ui->cmbIndenter->setCurrentIndex(indenterIndex);
    } else {
        ui->gbProperties->setTitle(i18n("Properties"));

        ui->gbProperties->setEnabled(false);
        ui->btnDelete->setEnabled(false);

        ui->edtName->clear();
        ui->edtSection->clear();
        ui->edtVariables->clear();
        ui->edtFileExtensions->clear();
        ui->edtMimeTypes->clear();
        ui->sbPriority->setValue(0);
        ui->cmbHl->setCurrentIndex(0);
        ui->cmbIndenter->setCurrentIndex(0);
    }

    m_lastType = type;
}

void ModeConfigPage::showMTDlg()
{
    QString text = i18n("Select the MimeTypes you want for this file type.\nPlease note that this will automatically edit the associated file extensions as well.");
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList list = ui->edtMimeTypes->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
#else
    QStringList list = ui->edtMimeTypes->text().split(QRegularExpression(QStringLiteral("\\s*;\\s*")), Qt::SkipEmptyParts);
#endif
    KMimeTypeChooserDialog d(i18n("Select Mime Types"), text, list, QStringLiteral("text"), this);
    if (d.exec() == QDialog::Accepted) {
        // do some checking, warn user if mime types or patterns are removed.
        // if the lists are empty, and the fields not, warn.
        ui->edtFileExtensions->setText(d.chooser()->patterns().join(QLatin1Char(';')));
        ui->edtMimeTypes->setText(d.chooser()->mimeTypes().join(QLatin1Char(';')));
    }
}

QString ModeConfigPage::name() const
{
    return i18n("Modes && Filetypes");
}

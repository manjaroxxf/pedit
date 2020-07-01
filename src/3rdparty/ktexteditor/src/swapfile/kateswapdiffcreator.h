/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2010-2018 Dominik Haumann <dhaumann@kde.org>

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

#ifndef KATE_SWAP_DIFF_CREATOR_H
#define KATE_SWAP_DIFF_CREATOR_H

#include <QAction>
#include <QByteArray>
#include <QProcess>
#include <QTemporaryFile>

namespace Kate
{
class SwapFile;
}

class SwapDiffCreator : public QObject
{
    Q_OBJECT

public:
    explicit SwapDiffCreator(Kate::SwapFile *swapFile);
    ~SwapDiffCreator();

public Q_SLOTS:
    void viewDiff();

private:
    Kate::SwapFile *const m_swapFile;

protected Q_SLOTS:
    void slotDataAvailable();
    void slotDiffFinished();

private:
    QProcess m_proc;
    QTemporaryFile m_originalFile;
    QTemporaryFile m_recoveredFile;
    QTemporaryFile m_diffFile;
};

#endif // KATE_SWAP_DIFF_CREATOR_H

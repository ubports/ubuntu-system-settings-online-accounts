/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of online-accounts-ui
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MOCK_UI_SERVER_H
#define MOCK_UI_SERVER_H

#include "ui-server.h"

#include <QObject>
#include <QString>

namespace OnlineAccountsUi {

class UiServerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(UiServer)

public:
    UiServerPrivate(const QString &address, UiServer *pluginServer);
    ~UiServerPrivate();
    static UiServerPrivate *mocked(UiServer *r) { return r->d_ptr; }

    void emitFinished() { Q_EMIT q_ptr->finished(); }

public:
    mutable UiServer *q_ptr;
    QString m_address;
};

} // namespace

#endif // MOCK_UI_SERVER_H

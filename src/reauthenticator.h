/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2012 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

#ifndef SIGNON_UI_REAUTHENTICATOR_H
#define SIGNON_UI_REAUTHENTICATOR_H

#include <QList>
#include <QObject>
#include <QVariantMap>

namespace SignOnUi {

struct AuthData {
    quint32 identity;
    QString method;
    QString mechanism;
    QVariantMap sessionData;
};

class ReauthenticatorPrivate;
class Reauthenticator: public QObject
{
    Q_OBJECT

public:
    Reauthenticator(const QList<AuthData> &clientData,
                    const QVariantMap &extraParameters,
                    QObject *parent = 0);
    ~Reauthenticator();

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void finished(bool success);

private:
    ReauthenticatorPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Reauthenticator)
};

} // namespace

#endif // SIGNON_UI_REAUTHENTICATOR_H


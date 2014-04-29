/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2011 Canonical Ltd.
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

#ifndef SIGNON_UI_SERVICE_H
#define SIGNON_UI_SERVICE_H

#include <QDBusContext>
#include <QObject>
#include <QVariantMap>

namespace SignOnUi {

class ServicePrivate;

class Service: public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.nokia.singlesignonui")

public:
    explicit Service(QObject *parent = 0);
    ~Service();

public Q_SLOTS:
    QVariantMap queryDialog(const QVariantMap &parameters);
    QVariantMap refreshDialog(const QVariantMap &newParameters);
    Q_NOREPLY void cancelUiRequest(const QString &requestId);
    void removeIdentityData(quint32 id);

private:
    ServicePrivate *d_ptr;
    Q_DECLARE_PRIVATE(Service)
};

} // namespace

#endif // SIGNON_UI_SERVICE_H


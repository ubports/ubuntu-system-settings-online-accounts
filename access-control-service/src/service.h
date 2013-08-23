/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of access-control-service
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

#ifndef ACS_SERVICE_H
#define ACS_SERVICE_H

#include <QDBusContext>
#include <QObject>
#include <QVariantMap>

namespace Acs {

class ServicePrivate;

class Service: public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(bool isIdle READ isIdle NOTIFY isIdleChanged)

public:
    explicit Service(QObject *parent = 0);
    ~Service();

    bool isIdle() const;

    QVariantMap requestAccess(const QVariantMap &options);

Q_SIGNALS:
    void isIdleChanged();

private:
    ServicePrivate *d_ptr;
    Q_DECLARE_PRIVATE(Service)
};

} // namespace

#endif // ACS_SERVICE_H


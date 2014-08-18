/*
 * Copyright (C) 2013 Canonical Ltd.
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

#ifndef OAU_REQUEST_H
#define OAU_REQUEST_H

#include <QDBusConnection>
#include <QDBusMessage>
#include <QObject>
#include <QVariantMap>

namespace OnlineAccountsUi {

class RequestPrivate;
class Request: public QObject
{
    Q_OBJECT

public:
    explicit Request(const QDBusConnection &connection,
                     const QDBusMessage &message,
                     const QVariantMap &parameters,
                     QObject *parent = 0);
    ~Request();

    static Request *find(const QVariantMap &match);

    quint64 windowId() const;
    void setInProgress(bool inProgress);
    bool isInProgress() const;
    const QVariantMap &parameters() const;
    QString clientApparmorProfile() const;
    QString interface() const;

public Q_SLOTS:
    void cancel();

Q_SIGNALS:
    void completed();

public Q_SLOTS:
    void fail(const QString &name, const QString &message);
    void setCanceled();
    void setResult(const QVariantMap &result);

private:
    RequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Request)
};

} // namespace

#endif // OAU_REQUEST_H

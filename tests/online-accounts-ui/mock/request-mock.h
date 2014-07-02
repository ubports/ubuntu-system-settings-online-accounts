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

#ifndef MOCK_REQUEST_H
#define MOCK_REQUEST_H

#include "request.h"

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace OnlineAccountsUi {

class RequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Request)

public:
    RequestPrivate(const QDBusConnection &connection,
                   const QDBusMessage &message,
                   const QVariantMap &parameters,
                   Request *request);
    ~RequestPrivate();
    static RequestPrivate *mocked(Request *r) { return r->d_ptr; }

    void setClientApparmorProfile(const QString &profile) {
        m_clientApparmorProfile = profile;
    }

Q_SIGNALS:
    void setWindowCalled(QWindow *);
    void failCalled(const QString &name, const QString &message);
    void setResultCalled(const QVariantMap &result);

private:
    mutable Request *q_ptr;
    QVariantMap m_parameters;
    QString m_clientApparmorProfile;
    QWindow *m_window;
    bool m_inProgress;
};

} // namespace

#endif // MOCK_REQUEST_H

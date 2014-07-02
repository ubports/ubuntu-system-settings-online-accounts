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

#ifndef MOCK_NOTIFICATiON_H
#define MOCK_NOTIFICATiON_H

#include "notification.h"

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

typedef QPair<QString,QString> ActionPair;

class NotificationPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Notification)

public:
    NotificationPrivate(const QString &summary,
                        const QString &body,
                        Notification *notification);
    ~NotificationPrivate();
    static NotificationPrivate *mocked(Notification *n) { return n->d_ptr; }

    static QList<Notification *> allNotifications;

    void invokeAction(const QString &action);

Q_SIGNALS:
    void showCalled();

public:
    QString m_summary;
    QString m_body;
    QList<ActionPair> m_actions;
    bool m_isSnapDecision;
    mutable Notification *q_ptr;
};

} // namespace

#endif // MOCK_NOTIFICATiON_H

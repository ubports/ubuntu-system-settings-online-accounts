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

#include "notification-mock.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

using namespace OnlineAccountsUi;

QList<Notification*> NotificationPrivate::allNotifications;

NotificationPrivate::NotificationPrivate(const QString &summary,
                                         const QString &body,
                                         Notification *notification):
    QObject(notification),
    m_summary(summary),
    m_body(body),
    q_ptr(notification)
{
    allNotifications.append(notification);
}

NotificationPrivate::~NotificationPrivate()
{
    allNotifications.removeAll(q_ptr);
}

void NotificationPrivate::invokeAction(const QString &action)
{
    Q_Q(Notification);
    Q_EMIT q->actionInvoked(action);
}

Notification::Notification(const QString &summary,
                           const QString &body,
                           QObject *parent):
    QObject(parent),
    d_ptr(new NotificationPrivate(summary, body, this))
{
}

Notification::~Notification()
{
}

void Notification::addAction(const QString &action, const QString &label)
{
    Q_D(Notification);
    d->m_actions.append(ActionPair(action, label));
}

void Notification::show()
{
    Q_D(Notification);
    Q_EMIT d->showCalled();
}

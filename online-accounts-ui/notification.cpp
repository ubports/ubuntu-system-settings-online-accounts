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

#include "debug.h"
#include "notification.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QString>
#include <libnotify/notification.h>
#include <libnotify/notify.h>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

class NotificationPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Notification)

public:
    NotificationPrivate(const QString &summary,
                        const QString &body,
                        Notification *notification);
    ~NotificationPrivate();

    static void actionCallback(NotifyNotification *, char *action,
                               Notification *q);

private:
    NotifyNotification *m_notification;
    mutable Notification *q_ptr;
};

} // namespace

static void onNotificationClosed(Notification *notification) {
    Q_EMIT notification->closed();
}

NotificationPrivate::NotificationPrivate(const QString &summary,
                                         const QString &body,
                                         Notification *notification):
    QObject(notification),
    m_notification(0),
    q_ptr(notification)
{
    if (!notify_is_initted()) {
        QByteArray name = QCoreApplication::applicationName().toUtf8();
        notify_init(name.constData());
    }

    QByteArray summaryUtf8 = summary.toUtf8();
    QByteArray bodyUtf8 = body.toUtf8();
    m_notification = notify_notification_new(summaryUtf8.constData(),
                                             bodyUtf8.constData(),
                                             NULL);
    g_signal_connect_swapped(m_notification, "closed",
                             G_CALLBACK(onNotificationClosed), notification);
}

NotificationPrivate::~NotificationPrivate()
{
    g_object_unref(m_notification);
}

void NotificationPrivate::actionCallback(NotifyNotification *, char *action,
                                         Notification *q)
{
    Q_EMIT q->actionInvoked(QString::fromUtf8(action));
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
    QByteArray actionUtf8 = action.toUtf8();
    QByteArray labelUtf8 = label.toUtf8();
    notify_notification_add_action(d->m_notification,
                                   actionUtf8, labelUtf8,
                                   NotifyActionCallback(
                                       &NotificationPrivate::actionCallback),
                                   this, NULL);
}

void Notification::setSnapDecision(bool snapDecision)
{
    Q_D(Notification);
    notify_notification_set_hint(d->m_notification,
                                 "x-canonical-snap-decisions",
                                 g_variant_new_boolean(snapDecision));
}

void Notification::show()
{
    Q_D(Notification);
    GError *error = NULL;
    if (!notify_notification_show(d->m_notification, &error)) {
        qWarning() << "Couldn't show notification:" << error->message;
        g_clear_error(&error);
    }
}

#include "notification.moc"

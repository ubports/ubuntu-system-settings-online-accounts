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

#ifndef OAU_NOTIFICATION_H
#define OAU_NOTIFICATION_H

#include <QObject>

namespace OnlineAccountsUi {

class NotificationPrivate;
class Notification: public QObject
{
    Q_OBJECT

public:
    explicit Notification(const QString &summary,
                          const QString &body,
                          QObject *parent = 0);
    ~Notification();

    void addAction(const QString &action, const QString &label);

public Q_SLOTS:
    void show();

Q_SIGNALS:
    void actionInvoked(const QString &action);
    void closed();

private:
    NotificationPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Notification)
};

} // namespace

#endif // OAU_NOTIFICATION_H

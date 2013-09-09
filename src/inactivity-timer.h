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

#ifndef OAU_INACTIVITY_TIMER_H
#define OAU_INACTIVITY_TIMER_H

#include <QList>
#include <QObject>
#include <QTimer>

namespace OnlineAccountsUi {

class InactivityTimer: public QObject
{
    Q_OBJECT

public:
    InactivityTimer(int interval, QObject *parent = 0);
    ~InactivityTimer() {}

    void watchObject(QObject *object);

Q_SIGNALS:
    void timeout();

private Q_SLOTS:
    void onIdleChanged();
    void onTimeout();

private:
    bool allObjectsAreIdle() const;

private:
    QList<QObject*> m_watchedObjects;
    QTimer m_timer;
    int m_interval;
};

} // namespace

#endif // OAU_INACTIVITY_TIMER_H

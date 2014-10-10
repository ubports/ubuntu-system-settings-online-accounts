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

#include "inactivity-timer.h"

#include <QDebug>
#include <QSignalSpy>
#include <QTest>

using namespace OnlineAccountsUi;

class Service: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isIdle READ isIdle NOTIFY isIdleChanged)

public:
    Service(): QObject(), m_isIdle(true) {}

    bool isIdle() const { return m_isIdle; }

    void setIdle(bool idle) {
        if (idle == m_isIdle) return;
        m_isIdle = idle;
        Q_EMIT isIdleChanged();
    }

Q_SIGNALS:
    void isIdleChanged();

private:
    bool m_isIdle;
};

class InactivityTimerTest: public QObject
{
    Q_OBJECT

public:
    InactivityTimerTest();

private Q_SLOTS:
    void testAlwaysIdle();
    void testBecomeIdle();
    void testManyServices();
};

InactivityTimerTest::InactivityTimerTest():
    QObject(0)
{
}

void InactivityTimerTest::testAlwaysIdle()
{
    InactivityTimer timer(10);
    QSignalSpy timeout(&timer, SIGNAL(timeout()));

    Service service;
    timer.watchObject(&service);

    QVERIFY(timeout.wait(100));
}

void InactivityTimerTest::testBecomeIdle()
{
    InactivityTimer timer(10);
    QSignalSpy timeout(&timer, SIGNAL(timeout()));

    Service service;
    service.setIdle(false);
    timer.watchObject(&service);

    /* No signal should be emitted, as the service is not idle */
    QVERIFY(!timeout.wait(100));

    service.setIdle(true);
    QVERIFY(timeout.wait(100));
}

void InactivityTimerTest::testManyServices()
{
    InactivityTimer timer(50);
    QSignalSpy timeout(&timer, SIGNAL(timeout()));

    Service service1;
    timer.watchObject(&service1);

    Service service2;
    timer.watchObject(&service2);

    Service service3;
    service3.setIdle(false);
    timer.watchObject(&service3);

    /* No signal should be emitted, as service3 is not idle */
    QVERIFY(!timeout.wait(100));

    /* Now set it is as idle, but soon afterwards set service1 as busy */
    service3.setIdle(true);
    QTest::qWait(10);
    service1.setIdle(false);
    QVERIFY(!timeout.wait(100));

    service1.setIdle(true);
    QVERIFY(timeout.wait(100));
}

QTEST_MAIN(InactivityTimerTest);

#include "tst_inactivity_timer.moc"

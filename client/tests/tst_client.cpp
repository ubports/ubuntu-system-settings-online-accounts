/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of OnlineAccountsClient.
 *
 * OnlineAccountsClient is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * OnlineAccountsClient is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OnlineAccountsClient.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <OnlineAccountsClient/Setup>
#include <QDebug>
#include <QFile>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QTest>

using namespace OnlineAccountsClient;

class SetupTest: public QObject
{
    Q_OBJECT

public:
    SetupTest();

private Q_SLOTS:
    void initTestCase();
    void testProperties();
    void testExec();
    void testExecWithProvider();
    void testExecWithServiceType();

private:
    QString parameters();
};

SetupTest::SetupTest():
    QObject(0)
{
}

QString SetupTest::parameters()
{
    QFile file("parameters.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("no file created!");
    }

    return QString::fromUtf8(file.readAll()).simplified();
}

void SetupTest::initTestCase()
{
    qputenv("PATH", MOCK_PATH);
}

void SetupTest::testProperties()
{
    Setup setup;

    QCOMPARE(setup.providerId(), QString());
    QCOMPARE(setup.serviceTypeId(), QString());

    setup.setProviderId("ciao");
    QCOMPARE(setup.providerId(), QString("ciao"));

    setup.setServiceTypeId("hello");
    QCOMPARE(setup.serviceTypeId(), QString("hello"));
}

void SetupTest::testExec()
{
    Setup setup;

    QSignalSpy finished(&setup, SIGNAL(finished()));
    setup.exec();

    QVERIFY(finished.wait());
    QCOMPARE(parameters(), QString("online-accounts"));
}

void SetupTest::testExecWithProvider()
{
    Setup setup;
    setup.setProviderId("lethal-provider");

    QSignalSpy finished(&setup, SIGNAL(finished()));
    setup.exec();

    QVERIFY(finished.wait());
    QCOMPARE(parameters(),
             QString("--option provider=lethal-provider online-accounts"));
}

void SetupTest::testExecWithServiceType()
{
    Setup setup;
    setup.setServiceTypeId("e-mail");

    QSignalSpy finished(&setup, SIGNAL(finished()));
    setup.exec();

    QVERIFY(finished.wait());
    QCOMPARE(parameters(),
             QString("--option serviceType=e-mail online-accounts"));
}

QTEST_MAIN(SetupTest);

#include "tst_client.moc"

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

#include <QDebug>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTest>

class SetupTest: public QObject
{
    Q_OBJECT

public:
    SetupTest();

private Q_SLOTS:
    void initTestCase();
    void testLoadPlugin();
    void testProperties();
    void testExec();
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
    qputenv("QML2_IMPORT_PATH", "../module");
    qputenv("PATH", MOCK_PATH);
}

void SetupTest::testLoadPlugin()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import Ubuntu.OnlineAccounts.Client 0.1\n"
                      "Setup {}",
                      QUrl());
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void SetupTest::testProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import Ubuntu.OnlineAccounts.Client 0.1\n"
                      "Setup { providerId: \"hello\" }",
                      QUrl());
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("providerId").toString(), QString("hello"));
    QCOMPARE(object->property("serviceTypeId").toString(), QString());

    object->setProperty("providerId", QString("ciao"));
    QCOMPARE(object->property("providerId").toString(), QString("ciao"));

    object->setProperty("serviceTypeId", QString("hi"));
    QCOMPARE(object->property("serviceTypeId").toString(), QString("hi"));

    delete object;
}

void SetupTest::testExec()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import Ubuntu.OnlineAccounts.Client 0.1\n"
                      "Setup {}",
                      QUrl());
    QObject *object = component.create();
    QVERIFY(object != 0);

    QSignalSpy finished(object, SIGNAL(finished()));
    QVERIFY(QMetaObject::invokeMethod(object, "exec"));

    QVERIFY(finished.wait());
    QCOMPARE(parameters(), QString("online-accounts"));
}

void SetupTest::testExecWithServiceType()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import Ubuntu.OnlineAccounts.Client 0.1\n"
                      "Setup { serviceTypeId: \"e-mail\" }",
                      QUrl());
    QObject *object = component.create();
    QVERIFY(object != 0);

    QSignalSpy finished(object, SIGNAL(finished()));
    QVERIFY(QMetaObject::invokeMethod(object, "exec"));

    QVERIFY(finished.wait());
    QCOMPARE(parameters(),
             QString("--option serviceType=e-mail online-accounts"));
}

QTEST_MAIN(SetupTest);

#include "tst_qml_client.moc"

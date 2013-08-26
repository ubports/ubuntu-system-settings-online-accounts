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

#include "access-control-service/globals.h"

#include <QDBusConnection>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QSignalSpy>
#include <QTest>

class Service: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface",
                "com.canonical.OnlineAccounts.AccessControl")

public:
    Service(): QObject() {}
    QVariantMap options() const { return m_options; }

public Q_SLOTS:
    QVariantMap requestAccess(const QVariantMap &options) {
        m_options = options;
        return QVariantMap();
    }

private:
    QVariantMap m_options;
};

class SetupTest: public QObject
{
    Q_OBJECT

public:
    SetupTest();
    QVariantMap options() const { return m_service.options(); }

private Q_SLOTS:
    void initTestCase();
    void testLoadPlugin();
    void testProperties();
    void testExec();
    void testExecWithServiceType();

private:
    Service m_service;
};

SetupTest::SetupTest():
    QObject(0)
{
}

void SetupTest::initTestCase()
{
    qputenv("QML2_IMPORT_PATH", PLUGIN_PATH);
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.registerObject(ACCESS_CONTROL_OBJECT_PATH,
                              &m_service,
                              QDBusConnection::ExportAllContents);
    connection.registerService(ACCESS_CONTROL_SERVICE_NAME);
}

void SetupTest::testLoadPlugin()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import Ubuntu.OnlineAccounts.Client 0.1\n"
                      "Setup {}",
                      QUrl());
    QObject *object = component.create();
    if (component.isError()) {
        Q_FOREACH(const QQmlError &error, component.errors()) {
            qDebug() << error;
        }
    }
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
    QCOMPARE(options().contains(ACS_KEY_PROVIDER), false);
    QCOMPARE(options().contains(ACS_KEY_SERVICE_TYPE), false);
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
    QCOMPARE(options().value(ACS_KEY_SERVICE_TYPE).toString(),
             QStringLiteral("e-mail"));
}

QTEST_MAIN(SetupTest);

#include "tst_qml_client.moc"

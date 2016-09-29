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
#include "globals.h"
#include "mock/application-manager-mock.h"
#include "mock/request-mock.h"
#include "mock/ui-server-mock.h"
#include "provider-request.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QSignalSpy>
#include <QTest>

using namespace OnlineAccountsUi;

namespace QTest {
template<>
char *toString(const QVariantMap &p)
{
    QJsonDocument doc(QJsonObject::fromVariantMap(p));
    QByteArray ba = doc.toJson(QJsonDocument::Compact);
    return qstrdup(ba.data());
}
} // QTest namespace

class TestRequest: public ProviderRequest
{
    Q_OBJECT

public:
    TestRequest(const QVariantMap &parameters, const QString &profile,
                QObject *parent = 0):
        ProviderRequest("interface", 0, profile, parameters, parent)
    {
    }
};

class ProviderRequestTest: public QObject
{
    Q_OBJECT

public:
    ProviderRequestTest();

private Q_SLOTS:
    void initTestCase();
    void testParameters_data();
    void testParameters();

private:
    UiServer m_uiServer;
};

ProviderRequestTest::ProviderRequestTest():
    QObject(),
    m_uiServer("fake")
{
}

void ProviderRequestTest::initTestCase()
{
    qputenv("ACCOUNTS", "/tmp/");
    qputenv("AG_APPLICATIONS", TEST_DATA_DIR);
    qputenv("AG_SERVICES", TEST_DATA_DIR);
    qputenv("AG_SERVICE_TYPES", TEST_DATA_DIR);
    qputenv("AG_PROVIDERS", TEST_DATA_DIR);
    qputenv("XDG_DATA_HOME", TEST_DATA_DIR);
}

void ProviderRequestTest::testParameters_data()
{
    QTest::addColumn<QVariantMap>("parameters");
    QTest::addColumn<QString>("profile");
    QTest::addColumn<QString>("providerId");
    QTest::addColumn<QVariantMap>("providerInfo");
    QTest::addColumn<QVariantMap>("applicationInfo");
    QTest::addColumn<QString>("errorName");

    QTest::newRow("empty") <<
        QVariantMap() <<
        QString() <<
        QString() <<
        QVariantMap() <<
        QVariantMap() <<
        OAU_ERROR_INVALID_APPLICATION;

    QVariantMap parameters;
    QVariantMap providerInfo;
    QVariantMap applicationInfo;

    parameters.insert(OAU_KEY_APPLICATION, "Gallery");
    parameters.insert(OAU_KEY_PROVIDER, "my provider");
    providerInfo.insert("some", "value");
    applicationInfo.insert("one", "two");
    QTest::newRow("confined app") <<
        parameters <<
        "my-app" <<
        "my provider" <<
        providerInfo <<
        applicationInfo <<
        "";
    parameters.clear();
    providerInfo.clear();
    applicationInfo.clear();

    parameters.insert(OAU_KEY_APPLICATION, "Gallery");
    parameters.insert(OAU_KEY_SERVICE_ID, "coolmail");
    providerInfo.insert("name", "cool");
    applicationInfo.insert("one", "two");
    QTest::newRow("by service ID") <<
        parameters <<
        "my-app" <<
        "cool" <<
        providerInfo <<
        applicationInfo <<
        "";
    parameters.clear();
    providerInfo.clear();
    applicationInfo.clear();
}

void ProviderRequestTest::testParameters()
{
    QFETCH(QVariantMap, parameters);
    QFETCH(QString, profile);
    QFETCH(QString, providerId);
    QFETCH(QVariantMap, providerInfo);
    QFETCH(QVariantMap, applicationInfo);
    QFETCH(QString, errorName);

    TestRequest request(parameters, profile);
    RequestPrivate *mockedRequest = RequestPrivate::mocked(&request);
    QSignalSpy setWindowCalled(mockedRequest,
                               SIGNAL(setWindowCalled(QWindow*)));
    QSignalSpy failCalled(mockedRequest,
                          SIGNAL(failCalled(const QString&, const QString&)));

    ApplicationManager *appManager = ApplicationManager::instance();
    ApplicationManagerPrivate *mockedAppManager =
        ApplicationManagerPrivate::mocked(appManager);
    mockedAppManager->setApplicationInfo(parameters[OAU_KEY_APPLICATION].toString(),
                                         applicationInfo);
    mockedAppManager->setProviderInfo(providerId, providerInfo);
    QSignalSpy applicationInfoCalled(mockedAppManager,
                                     SIGNAL(applicationInfoCalled(QString,QString)));

    request.start();

    if (errorName.isEmpty()) {
        QTRY_COMPARE(setWindowCalled.count(), 1);
        QQuickView *view = static_cast<QQuickView*>(setWindowCalled.at(0).at(0).value<QWindow*>());
        QQmlContext *context = view->engine()->rootContext();
        QObject *request = context->contextProperty("request").value<QObject*>();

        QCOMPARE(applicationInfoCalled.count(), 1);
        QCOMPARE(applicationInfoCalled.at(0).at(1).toString(), profile);

        QCOMPARE(request->property("provider").toMap(), providerInfo);
        QCOMPARE(request->property("application").toMap(), applicationInfo);
    } else {
        QTRY_COMPARE(failCalled.count(), 1);
        QCOMPARE(failCalled.at(0).at(0).toString(), errorName);
    }
}

QTEST_MAIN(ProviderRequestTest);

#include "tst_provider_request.moc"

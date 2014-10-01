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

#include "browser-request.h"
#include "debug.h"
#include "globals.h"
#include "request-handler.h"
#include "mock/request-mock.h"
#include "mock/signonui-request-mock.h"
#include "mock/ui-server-mock.h"

#include <QDebug>
#include <QNetworkCookie>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>

using namespace OnlineAccountsUi;

class TestRequest: public SignOnUi::BrowserRequest
{
    Q_OBJECT

public:
    TestRequest(const QVariantMap &parameters, QObject *parent = 0):
        SignOnUi::BrowserRequest(0, "profile", parameters, parent)
    {
    }
};

class BrowserRequestTest: public QObject
{
    Q_OBJECT

public:
    BrowserRequestTest();

private Q_SLOTS:
    void initTestCase();
    void testParametersWithHandler_data();
    void testParametersWithHandler();
    void testSuccessWithHandler();
    void testFailureWithHandler();
    void testCancelWithHandler();

private:
    QTemporaryDir m_dataDir;
    UiServer m_uiServer;
};

BrowserRequestTest::BrowserRequestTest():
    QObject(),
    m_uiServer("fake")
{
}

void BrowserRequestTest::initTestCase()
{
    QVERIFY(m_dataDir.isValid());

    qputenv("XDG_CACHE_HOME", m_dataDir.path().toUtf8());
}

void BrowserRequestTest::testParametersWithHandler_data()
{
    QTest::addColumn<QVariantMap>("parameters");
    QTest::addColumn<QString>("pageComponentUrl");
    QTest::addColumn<QString>("startUrl");
    QTest::addColumn<QString>("finalUrl");
    QTest::addColumn<QString>("rootDir");

    QString baseCacheDir = QString("file://%1/tst_browser_request").
        arg(m_dataDir.path());

    QTest::newRow("empty") <<
        QVariantMap() <<
        "DefaultPage.qml" <<
        QString() <<
        QString() <<
        baseCacheDir + "/id-0";

    QVariantMap parameters;
    QVariantMap clientData;
    parameters.insert(SSOUI_KEY_OPENURL, "http://localhost/start.html");
    parameters.insert(SSOUI_KEY_FINALURL, "http://localhost/end.html");
    parameters.insert(SSOUI_KEY_IDENTITY, uint(4));
    QTest::newRow("with URLs and ID") <<
        parameters <<
        "DefaultPage.qml" <<
        "http://localhost/start.html" <<
        "http://localhost/end.html" <<
        baseCacheDir + "/id-4";
    parameters.clear();

    clientData.insert("X-PageComponent",
                      QUrl("file:///usr/share/signon-ui/MyPage.qml"));
    parameters.insert(SSOUI_KEY_CLIENT_DATA, clientData);
    parameters.insert(SSOUI_KEY_OPENURL, "http://localhost/start.html");
    parameters.insert(SSOUI_KEY_FINALURL, "http://localhost/end.html");
    parameters.insert(SSOUI_KEY_IDENTITY, uint(4));
    QTest::newRow("with page component") <<
        parameters <<
        "file:///usr/share/signon-ui/MyPage.qml" <<
        "http://localhost/start.html" <<
        "http://localhost/end.html" <<
        baseCacheDir + "/id-4";
    parameters.clear();
}

void BrowserRequestTest::testParametersWithHandler()
{
    QFETCH(QVariantMap, parameters);
    QFETCH(QString, pageComponentUrl);
    QFETCH(QString, startUrl);
    QFETCH(QString, finalUrl);
    QFETCH(QString, rootDir);

    SignOnUi::RequestHandler handler;
    QSignalSpy requestChanged(&handler, SIGNAL(requestChanged()));

    TestRequest request(parameters);
    request.setHandler(&handler);

    request.start();

    QCOMPARE(requestChanged.count(), 1);
    QObject *req = handler.request();

    QCOMPARE(req->property("pageComponentUrl").toUrl().toString(), pageComponentUrl);
    QCOMPARE(req->property("currentUrl").toUrl().toString(), QString());
    QCOMPARE(req->property("startUrl").toUrl().toString(), startUrl);
    QCOMPARE(req->property("finalUrl").toUrl().toString(), finalUrl);
    QCOMPARE(req->property("rootDir").toString(), rootDir);
}

void BrowserRequestTest::testSuccessWithHandler()
{
    SignOnUi::RequestHandler handler;
    QSignalSpy requestChanged(&handler, SIGNAL(requestChanged()));

    QVariantMap parameters;
    parameters.insert(SSOUI_KEY_OPENURL, "http://localhost/start.html");
    parameters.insert(SSOUI_KEY_FINALURL, "http://localhost/end.html");
    parameters.insert(SSOUI_KEY_IDENTITY, uint(4));
    TestRequest request(parameters);
    QSignalSpy completed(&request, SIGNAL(completed()));

    OnlineAccountsUi::RequestPrivate *mockedRequest =
        OnlineAccountsUi::RequestPrivate::mocked(&request);
    QSignalSpy setResultCalled(mockedRequest,
                               SIGNAL(setResultCalled(const QVariantMap &)));

    request.setHandler(&handler);
    request.start();

    QCOMPARE(requestChanged.count(), 1);
    QObject *req = handler.request();
    QSignalSpy authenticated(req, SIGNAL(authenticated()));

    /* Go through a couple of pages */
    QMetaObject::invokeMethod(req, "onLoadStarted");
    QMetaObject::invokeMethod(req, "onLoadFinished", Q_ARG(bool, true));
    req->setProperty("currentUrl", QUrl("http://localhost/somewhere"));
    QCOMPARE(authenticated.count(), 0);

    /* Pretend a page is failing, but then go to the destination URL before the
     * fail timer is triggered */
    QMetaObject::invokeMethod(req, "onLoadStarted");
    QMetaObject::invokeMethod(req, "onLoadFinished", Q_ARG(bool, false));
    req->setProperty("currentUrl", QUrl("http://localhost/somewhere-else"));
    QCOMPARE(authenticated.count(), 0);

    /* Finally, arrive at the destination URL */
    QMetaObject::invokeMethod(req, "onLoadStarted");
    QMetaObject::invokeMethod(req, "onLoadFinished", Q_ARG(bool, true));
    req->setProperty("currentUrl", QUrl("http://localhost/end.html?code=ciao"));
    QCOMPARE(authenticated.count(), 1);
    QCOMPARE(completed.count(), 0);

    /* Store some cookies */
    QVariantList cookieList;
    QList<QNetworkCookie> expectedCookies = QNetworkCookie::parseCookies(
        "a=1\nb=2");
    Q_FOREACH(const QNetworkCookie &c, expectedCookies) {
        cookieList.append(QVariant::fromValue(c));
    }
    QVariant cookies(cookieList);
    QMetaObject::invokeMethod(req, "setCookies", Qt::QueuedConnection,
                              Q_ARG(QVariant, cookies));
    QVERIFY(completed.wait());
    QCOMPARE(completed.count(), 1);

    QCOMPARE(setResultCalled.count(), 1);
    QVariantMap results = setResultCalled.at(0).at(0).toMap();
    QCOMPARE(results.value(SSOUI_KEY_URLRESPONSE).toString(),
             QString("http://localhost/end.html?code=ciao"));
}

void BrowserRequestTest::testFailureWithHandler()
{
    SignOnUi::RequestHandler handler;
    QSignalSpy requestChanged(&handler, SIGNAL(requestChanged()));

    QVariantMap parameters;
    parameters.insert(SSOUI_KEY_OPENURL, "http://localhost/start.html");
    parameters.insert(SSOUI_KEY_FINALURL, "http://localhost/end.html");
    parameters.insert(SSOUI_KEY_IDENTITY, uint(4));
    TestRequest request(parameters);
    QSignalSpy completed(&request, SIGNAL(completed()));

    OnlineAccountsUi::RequestPrivate *mockedRequest =
        OnlineAccountsUi::RequestPrivate::mocked(&request);
    QSignalSpy setResultCalled(mockedRequest,
                               SIGNAL(setResultCalled(const QVariantMap &)));

    request.setHandler(&handler);
    request.start();

    QCOMPARE(requestChanged.count(), 1);
    QObject *req = handler.request();
    QSignalSpy authenticated(req, SIGNAL(authenticated()));

    /* Fail to load a page */
    QMetaObject::invokeMethod(req, "onLoadStarted");
    QMetaObject::invokeMethod(req, "onLoadFinished", Q_ARG(bool, false));

    QVERIFY(completed.wait());
    QCOMPARE(completed.count(), 1);
    QCOMPARE(authenticated.count(), 0);

    QCOMPARE(setResultCalled.count(), 1);
    QVariantMap results = setResultCalled.at(0).at(0).toMap();
    QVERIFY(results.isEmpty());
}

void BrowserRequestTest::testCancelWithHandler()
{
    SignOnUi::RequestHandler handler;
    QSignalSpy requestChanged(&handler, SIGNAL(requestChanged()));

    QVariantMap parameters;
    parameters.insert(SSOUI_KEY_OPENURL, "http://localhost/start.html");
    parameters.insert(SSOUI_KEY_FINALURL, "http://localhost/end.html");
    parameters.insert(SSOUI_KEY_IDENTITY, uint(4));
    TestRequest request(parameters);
    QSignalSpy completed(&request, SIGNAL(completed()));

    OnlineAccountsUi::RequestPrivate *mockedRequest =
        OnlineAccountsUi::RequestPrivate::mocked(&request);
    QSignalSpy setResultCalled(mockedRequest,
                               SIGNAL(setResultCalled(const QVariantMap &)));

    request.setHandler(&handler);
    request.start();

    QCOMPARE(requestChanged.count(), 1);
    requestChanged.clear();
    QObject *req = handler.request();
    QSignalSpy authenticated(req, SIGNAL(authenticated()));

    /* Start loading a page, then cancel */
    QMetaObject::invokeMethod(req, "onLoadStarted");
    QMetaObject::invokeMethod(req, "cancel", Qt::QueuedConnection);

    QVERIFY(requestChanged.wait());
    QCOMPARE(requestChanged.count(), 1);
    QVERIFY(!handler.request());

    QCOMPARE(completed.count(), 1);
    QCOMPARE(authenticated.count(), 0);

    QCOMPARE(setResultCalled.count(), 1);
    QVariantMap results = setResultCalled.at(0).at(0).toMap();
    QCOMPARE(results.value(SSOUI_KEY_ERROR).toInt(),
             int(SignOn::QUERY_ERROR_CANCELED));
}

QTEST_MAIN(BrowserRequestTest);

#include "tst_browser_request.moc"

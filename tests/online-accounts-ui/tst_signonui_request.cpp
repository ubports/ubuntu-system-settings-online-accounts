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

#include "globals.h"
#include "signonui-request.h"
#include "mock/request-mock.h"
#include "mock/ui-server-mock.h"

#include <Accounts/Account>
#include <Accounts/Manager>

#include <OnlineAccountsPlugin/request-handler.h>

#include <QDebug>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>

using namespace OnlineAccountsUi;

class TestRequest: public SignOnUi::Request
{
    Q_OBJECT

public:
    TestRequest(const QString &clientProfile,
                const QVariantMap &parameters,
                QObject *parent = 0):
        Request(0, clientProfile, parameters, parent)
    {
    }

    void setWindow(QWindow *window) { Request::setWindow(window); }

    void sendResult(const QVariantMap &result) { setResult(result); }
};

class SignonuiRequestTest: public QObject
{
    Q_OBJECT

public:
    SignonuiRequestTest();

private Q_SLOTS:
    void initTestCase();
    void testParameters_data();
    void testParameters();
    void testHandler();

private:
    bool mustCreateAccount(uint credentialsId) { return credentialsId > 10; }

private:
    QTemporaryDir m_accountsDir;
    UiServer m_uiServer;
};

SignonuiRequestTest::SignonuiRequestTest():
    QObject(),
    m_uiServer("fake")
{
}

void SignonuiRequestTest::initTestCase()
{
    QVERIFY(m_accountsDir.isValid());

    qputenv("ACCOUNTS", m_accountsDir.path().toUtf8());
    qputenv("AG_APPLICATIONS", TEST_DATA_DIR);
    qputenv("AG_PROVIDERS", TEST_DATA_DIR);
    qputenv("AG_SERVICES", TEST_DATA_DIR);
    qputenv("XDG_DATA_HOME", TEST_DATA_DIR);
}

void SignonuiRequestTest::testParameters_data()
{
    QTest::addColumn<QVariantMap>("parameters");
    QTest::addColumn<uint>("identity");
    QTest::addColumn<QString>("method");
    QTest::addColumn<QString>("mechanism");
    QTest::addColumn<QString>("ssoId");
    QTest::addColumn<QVariantMap>("clientData");

    QTest::newRow("empty") <<
        QVariantMap() <<
        uint(0) <<
        QString() <<
        QString() <<
        QString() <<
        QVariantMap();

    QVariantMap clientData;
    clientData.insert("some number", 4);

    QVariantMap parameters;
    parameters.insert(SSOUI_KEY_CLIENT_DATA, clientData);
    parameters.insert(SSOUI_KEY_IDENTITY, uint(45));
    parameters.insert(SSOUI_KEY_METHOD, QString("a method"));
    parameters.insert(SSOUI_KEY_MECHANISM, QString("a mechanism"));
    parameters.insert(SSOUI_KEY_REQUESTID, QString("/id-4"));


    QTest::newRow("basic") <<
        parameters <<
        uint(45) <<
        "a method" <<
        "a mechanism" <<
        "/id-4" <<
        clientData;
}

void SignonuiRequestTest::testParameters()
{
    QFETCH(QVariantMap, parameters);
    QFETCH(QString, method);
    QFETCH(QString, mechanism);
    QFETCH(QString, ssoId);
    QFETCH(QVariantMap, clientData);

    TestRequest request("unconfined", parameters);
    QCOMPARE(request.method(), method);
    QCOMPARE(request.mechanism(), mechanism);
    QCOMPARE(request.ssoId(), ssoId);
    QCOMPARE(request.clientData(), clientData);
}

void SignonuiRequestTest::testHandler()
{
    QVariantMap parameters;
    TestRequest request("unconfined", parameters);
    QVERIFY(request.handler() == 0);

    SignOnUi::RequestHandler *handler = new SignOnUi::RequestHandler;
    request.setHandler(handler);
    QCOMPARE(request.handler(), handler);

    /* Try to set another handler; this won't be allowed */
    SignOnUi::RequestHandler *handler2 = new SignOnUi::RequestHandler;
    request.setHandler(handler2);
    QCOMPARE(request.handler(), handler);

    delete handler2;
    delete handler;
}

QTEST_MAIN(SignonuiRequestTest);

#include "tst_signonui_request.moc"

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
#include "request-handler.h"
#include "signonui-request.h"
#include "mock/notification-mock.h"
#include "mock/request-mock.h"

#include <Accounts/Account>
#include <Accounts/Manager>

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
    TestRequest(const QDBusConnection &connection,
                const QDBusMessage &message,
                const QVariantMap &parameters,
                QObject *parent = 0):
        Request(connection, message, parameters, parent)
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
    void testSnapDecision_data();
    void testSnapDecision();

private:
    bool mustCreateAccount(uint credentialsId) { return credentialsId > 10; }

private:
    QDBusConnection m_connection;
    QDBusMessage m_message;
    QTemporaryDir m_accountsDir;
};

SignonuiRequestTest::SignonuiRequestTest():
    QObject(),
    m_connection(QStringLiteral("uninitialized"))
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
    QTest::addColumn<QString>("id");
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
    QFETCH(QString, id);
    QFETCH(QVariantMap, clientData);

    TestRequest request(m_connection, m_message, parameters);
    QCOMPARE(request.method(), method);
    QCOMPARE(request.mechanism(), mechanism);
    QCOMPARE(request.id(), id);
    QCOMPARE(request.clientData(), clientData);
}

void SignonuiRequestTest::testHandler()
{
    QVariantMap parameters;
    TestRequest request(m_connection, m_message, parameters);
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

void SignonuiRequestTest::testSnapDecision_data()
{
    QTest::addColumn<uint>("credentialsId");
    QTest::addColumn<QString>("accountName");
    QTest::addColumn<QString>("clientProfile");
    QTest::addColumn<QString>("applicationName");
    QTest::addColumn<bool>("mustAccept");
    QTest::addColumn<QVariantMap>("result");

    QVariantMap acceptedResult;
    acceptedResult.insert("some key", QString("some value"));

    QVariantMap declinedResult;
    declinedResult.insert(SSOUI_KEY_ERROR, SignOn::QUERY_ERROR_CANCELED);

    QVariantMap errorResult;
    errorResult.insert(SSOUI_KEY_ERROR, SignOn::QUERY_ERROR_FORBIDDEN);

    QTest::newRow("no account") <<
        uint(0) <<
        "tom@example.com" <<
        "com.ubuntu.tests_application_0.3" <<
        QString() <<
        false <<
        errorResult;

    QTest::newRow("invalid account") <<
        uint(1) <<
        "tom@example.com" <<
        "com.ubuntu.tests_application_0.3" <<
        QString() <<
        false <<
        errorResult;

    QTest::newRow("valid application, accepted") <<
        uint(14231) <<
        "tom@example.com" <<
        "com.ubuntu.tests_application_0.3" <<
        "Easy Mailer" <<
        true <<
        acceptedResult;

    QTest::newRow("valid application, declined") <<
        uint(14231) <<
        "tom@example.com" <<
        "com.ubuntu.tests_application_0.3" <<
        "Easy Mailer" <<
        false <<
        declinedResult;

    QTest::newRow("unconfined application, accepted") <<
        uint(14235) <<
        "tom@example.com" <<
        "unconfined" <<
        "Ubuntu" <<
        true <<
        acceptedResult;

    QTest::newRow("unconfined application, declined") <<
        uint(14235) <<
        "tom@example.com" <<
        "unconfined" <<
        "Ubuntu" <<
        false <<
        declinedResult;
}

void SignonuiRequestTest::testSnapDecision()
{
    QString providerId("cool");
    QFETCH(uint, credentialsId);
    QFETCH(QString, accountName);
    QFETCH(QString, clientProfile);
    QFETCH(QString, applicationName);
    QFETCH(bool, mustAccept);
    QFETCH(QVariantMap, result);

    // First, create an account
    Accounts::Manager *manager = new Accounts::Manager(this);
    Accounts::Provider provider = manager->provider(providerId);
    QVERIFY(provider.isValid());
    if (mustCreateAccount(credentialsId)) {
        Accounts::Account *account = manager->createAccount(providerId);
        QVERIFY(account != 0);
        account->setEnabled(true);
        account->setDisplayName(accountName);
        account->setCredentialsId(credentialsId);
        account->syncAndBlock();
    }

    /* Then, create a request referring to the same credentials ID of the
     * created account. */
    QVariantMap parameters;
    parameters.insert(SSOUI_KEY_IDENTITY, credentialsId);
    parameters.insert(SSOUI_KEY_METHOD, "funnyMethod");
    parameters.insert(SSOUI_KEY_MECHANISM, "funnyMechanism");
    TestRequest request(m_connection, m_message, parameters);
    OnlineAccountsUi::RequestPrivate *mockRequest =
        OnlineAccountsUi::RequestPrivate::mocked(&request);
    QSignalSpy failCalled(mockRequest,
                          SIGNAL(failCalled(const QString&, const QString&)));
    QSignalSpy setResultCalled(mockRequest,
                               SIGNAL(setResultCalled(const QVariantMap &)));
    mockRequest->setClientApparmorProfile(clientProfile);
    request.start();

    /* Request to show a window; a snap decision should appear instead */
    QWindow *window = new QWindow;
    QSignalSpy setWindowCalled(mockRequest,
                               SIGNAL(setWindowCalled(QWindow*)));
    request.setWindow(window);
    QCOMPARE(setWindowCalled.count(), 0);
    if (mustCreateAccount(credentialsId)) {
        QCOMPARE(NotificationPrivate::allNotifications.count(), 1);
    } else {
        /* If the account is not found, no notification should appear, and an
         * error returned to the app */
        QCOMPARE(NotificationPrivate::allNotifications.count(), 0);
        QCOMPARE(setResultCalled.count(), 1);
        QCOMPARE(setResultCalled.at(0).at(0).toMap(), result);
        return;
    }

    /* Inspect the snap decision contents */
    Notification *notification =
        NotificationPrivate::allNotifications.first();
    NotificationPrivate *mockNotification =
        NotificationPrivate::mocked(notification);
    QCOMPARE(mockNotification->m_summary, QString("Authentication request"));
    QCOMPARE(mockNotification->m_body,
             QString("Please authorize %1 to access your %2 account %3").
             arg(applicationName).arg(provider.displayName()).arg(accountName));
    QVERIFY(mockNotification->m_isSnapDecision);

    /* Invoke the action on the snap decision */
    QString action = mustAccept ? "continue" : "cancel";
    QSignalSpy actionInvoked(notification,
                             SIGNAL(actionInvoked(const QString &)));
    mockNotification->invokeAction(action);
    QCOMPARE(actionInvoked.count(), 1);
    QCOMPARE(actionInvoked.at(0).at(0).toString(), action);

    /* Here we iterate the main loop because the notification object is
     * destroyed with deleteLater() */
    QTest::qWait(5);

    if (mustAccept) {
        QCOMPARE(setWindowCalled.count(), 1);
        QCOMPARE(setWindowCalled.at(0).at(0), QVariant::fromValue(window));
        QCOMPARE(failCalled.count(), 0);
        QCOMPARE(setResultCalled.count(), 0);

        /* deliver the result */
        request.sendResult(result);
        QCOMPARE(setResultCalled.count(), 1);
        QCOMPARE(setResultCalled.at(0).at(0).toMap(), result);
    } else {
        QCOMPARE(setWindowCalled.count(), 0);
        QCOMPARE(failCalled.count(), 0);
        QCOMPARE(setResultCalled.count(), 1);
        QCOMPARE(setResultCalled.at(0).at(0).toMap(), result);
    }
    delete window;
    delete manager;
}

QTEST_MAIN(SignonuiRequestTest);

#include "tst_signonui_request.moc"

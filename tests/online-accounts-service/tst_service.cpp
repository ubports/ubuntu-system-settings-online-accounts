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

#include "globals.h"
#include "request.h"
#include "request-manager.h"
#include "service.h"
#include "ui-proxy.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServer>
#include <QDebug>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QTimer>

using namespace OnlineAccountsUi;

#define P2P_SOCKET "unix:path=/tmp/tst_service_%1"
#define TEST_SERVICE_NAME \
    QStringLiteral("com.ubuntu.OnlineAccountsUi.Test")
#define TEST_OBJECT_PATH QStringLiteral("/")

QList<UiProxyPrivate *> m_uiProxies;

class RequestReply: public QDBusPendingCallWatcher
{
    Q_OBJECT

public:
    RequestReply(const QDBusPendingCall &call, QObject *parent = 0):
        QDBusPendingCallWatcher(call, parent),
        m_isError(false)
    {
        QObject::connect(this, SIGNAL(finished(QDBusPendingCallWatcher*)),
                         this, SLOT(onFinished()));
    }

    bool isError() const { return m_isError; }
    QVariantMap reply() const { return m_reply; }
    QString errorName() const { return m_errorName; }
    QString errorMessage() const { return m_errorMessage; }

private Q_SLOTS:
    void onFinished() {
        QDBusPendingReply<QVariantMap> reply = *this;
        if (reply.isError()) {
            m_isError = true;
            m_errorName = reply.error().name();
            m_errorMessage = reply.error().message();
        } else {
            m_reply = qdbus_cast<QVariantMap>(reply.argumentAt(0).
                                              value<QDBusArgument>());
        }
        Q_EMIT finished();
    }

Q_SIGNALS:
    void finished();

private:
    bool m_isError;
    QVariantMap m_reply;
    QString m_errorName;
    QString m_errorMessage;
};

class ServiceTest: public QObject
{
    Q_OBJECT

public:
    ServiceTest();

private:
    RequestReply *sendRequest(const QVariantMap &parameters) {
        QDBusMessage msg =
            QDBusMessage::createMethodCall(TEST_SERVICE_NAME,
                                           TEST_OBJECT_PATH,
                                           "com.ubuntu.OnlineAccountsUi",
                                           "requestAccess");
        msg.setArguments(QVariantList() << parameters);
        QDBusPendingCall call = m_connection.asyncCall(msg);
        return new RequestReply(call, this);
    }

private Q_SLOTS:
    void initTestCase();
    void testResults();
    void testFailure();
    void testIdle();

protected Q_SLOTS:
    void onNewConnection(const QDBusConnection &connection);

private:
    RequestManager m_requestManager;
    Service m_service;
    QDBusConnection m_connection;
};

/* Mocking UiProxy { */
namespace OnlineAccountsUi {

class UiProxyPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(UiProxy)

public:
    UiProxyPrivate(UiProxy *uiProxy):
        QObject(uiProxy),
        m_initCount(0),
        m_initReply(true),
        q_ptr(uiProxy)
    {
    }
    ~UiProxyPrivate() {};

    void emitFinished() { Q_EMIT q_ptr->finished(); }

Q_SIGNALS:
    void handleRequestCalled();

public:
    int m_initCount;
    bool m_initReply;
    QList<Request*> m_requests;
    QVariantMap m_expectedHasHandlerFor;
    mutable UiProxy *q_ptr;
};

} // namespace

UiProxy::UiProxy(pid_t, QObject *parent):
    QObject(parent),
    d_ptr(new UiProxyPrivate(this))
{
    m_uiProxies.append(d_ptr);
}

UiProxy::~UiProxy()
{
    m_uiProxies.removeOne(d_ptr);
}

bool UiProxy::init()
{
    Q_D(UiProxy);
    d->m_initCount++;
    return d->m_initReply;
}

void UiProxy::handleRequest(Request *request)
{
    Q_D(UiProxy);
    d->m_requests.append(request);
    Q_EMIT d->handleRequestCalled();
}

bool UiProxy::hasHandlerFor(const QVariantMap &parameters)
{
    Q_D(UiProxy);
    return parameters == d->m_expectedHasHandlerFor;
}

/* } mocking UiProxy */

ServiceTest::ServiceTest():
    QObject(0),
    m_connection(QStringLiteral("uninitialized"))
{
}

void ServiceTest::onNewConnection(const QDBusConnection &connection)
{
    QDBusConnection conn(connection);
    conn.registerService(TEST_SERVICE_NAME);
    conn.registerObject(TEST_OBJECT_PATH, &m_service);
}

void ServiceTest::initTestCase()
{
    QDBusServer *server;
    for (int i = 0; i < 10; i++) {
        server = new QDBusServer(QString::fromLatin1(P2P_SOCKET).arg(i), this);
        if (!server->isConnected()) {
            delete server;
        } else {
            break;
        }
    }

    QVERIFY(server->isConnected());

    QObject::connect(server, SIGNAL(newConnection(const QDBusConnection &)),
                     this, SLOT(onNewConnection(const QDBusConnection &)));

    m_connection = QDBusConnection::connectToPeer(server->address(),
                                                  QStringLiteral("tst"));
    QTest::qWait(10);
    QVERIFY(m_connection.isConnected());
}

void ServiceTest::testResults()
{
    QVariantMap parameters;
    parameters.insert("hello", QString("world"));
    RequestReply *call = sendRequest(parameters);
    QSignalSpy callFinished(call, SIGNAL(finished()));

    QTRY_COMPARE(m_uiProxies.count(), 1);

    UiProxyPrivate *proxy = m_uiProxies[0];
    QCOMPARE(proxy->m_initCount, 1);
    QCOMPARE(proxy->m_requests.count(), 1);

    Request *request = proxy->m_requests.last();
    QCOMPARE(request->parameters(), parameters);

    request->setInProgress(true);
    request->setResult(parameters);

    QVERIFY(callFinished.wait());
    QCOMPARE(call->isError(), false);
    QCOMPARE(call->reply(), parameters);
    delete call;

    proxy->emitFinished();
    QTRY_COMPARE(m_uiProxies.count(), 0);
}

void ServiceTest::testFailure()
{
    QVariantMap parameters;
    parameters.insert("hi", "there");
    RequestReply *call = sendRequest(parameters);
    QSignalSpy callFinished(call, SIGNAL(finished()));

    QTRY_COMPARE(m_uiProxies.count(), 1);

    UiProxyPrivate *proxy = m_uiProxies[0];
    QCOMPARE(proxy->m_initCount, 1);
    QCOMPARE(proxy->m_requests.count(), 1);

    Request *request = proxy->m_requests.last();
    QCOMPARE(request->parameters(), parameters);

    request->setInProgress(true);
    QString errorName("com.ubuntu.OnlineAccountsUi.BadLuck");
    QString errorMessage("really unlucky");
    request->fail(errorName, errorMessage);

    QVERIFY(callFinished.wait());
    QCOMPARE(call->isError(), true);
    QCOMPARE(call->errorName(), errorName);
    QCOMPARE(call->errorMessage(), errorMessage);
    delete call;

    proxy->emitFinished();
    QTRY_COMPARE(m_uiProxies.count(), 0);
}

void ServiceTest::testIdle()
{
    QCOMPARE(m_requestManager.isIdle(), true);

    QSignalSpy isIdleChanged(&m_requestManager, SIGNAL(isIdleChanged()));

    QVariantMap parameters;
    parameters.insert("time", "out");
    RequestReply *call = sendRequest(parameters);
    QSignalSpy callFinished(call, SIGNAL(finished()));

    QVERIFY(isIdleChanged.wait());
    QCOMPARE(m_requestManager.isIdle(), false);
    isIdleChanged.clear();

    QTRY_COMPARE(m_uiProxies.count(), 1);

    UiProxyPrivate *proxy = m_uiProxies[0];
    QCOMPARE(proxy->m_initCount, 1);
    QCOMPARE(proxy->m_requests.count(), 1);

    Request *request = proxy->m_requests.last();
    QCOMPARE(request->parameters(), parameters);

    request->setInProgress(true);
    request->setResult(parameters);

    /* the request will terminate, so expect the service
     * to be idle again */
    QTRY_COMPARE(isIdleChanged.count(), 1);
    QCOMPARE(m_requestManager.isIdle(), true);

    QVERIFY(callFinished.wait());
    QCOMPARE(call->isError(), false);
    delete call;

    proxy->emitFinished();
    QTRY_COMPARE(m_uiProxies.count(), 0);
}

QTEST_MAIN(ServiceTest);

#include "tst_service.moc"

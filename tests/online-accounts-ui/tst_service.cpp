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
#include "window-watcher.h"

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

static const QString keyTimeout(QStringLiteral("timeout"));
static const QString keyFail(QStringLiteral("fail"));

#define P2P_SOCKET "unix:path=/tmp/tst_service_%1"
#define TEST_SERVICE_NAME \
    QStringLiteral("com.ubuntu.OnlineAccountsUi.Test")
#define TEST_OBJECT_PATH QStringLiteral("/")

class TestRequest: public Request
{
    Q_OBJECT

public:
    TestRequest(const QDBusConnection &connection,
                const QDBusMessage &message,
                const QVariantMap &parameters,
                QObject *parent = 0):
        Request(connection, message, parameters, parent)
    {
        m_timer.setSingleShot(true);
        m_timer.setInterval(parameters.value(keyTimeout).toInt());
        QObject::connect(&m_timer, SIGNAL(timeout()),
                         this, SLOT(onTimeout()));
    }

    void start() Q_DECL_OVERRIDE {
        Request::start();
        QWindow *window = new QWindow;
        setWindow(window);
        m_timer.start();
    }

private Q_SLOTS:
    void onTimeout() {
        if (parameters().contains(keyFail)) {
            fail(parameters().value(keyFail).toString(), "Request failed");
        } else {
            setResult(parameters());
        }
    }

private:
    QTimer m_timer;
};

Request *Request::newRequest(const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent)
{
    return new TestRequest(connection, message, parameters, parent);
}

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

private Q_SLOTS:
    void onFinished() {
        QDBusPendingReply<QVariantMap> reply = *this;
        if (reply.isError()) {
            m_isError = true;
            m_errorName = reply.error().name();
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
    void testWindow();
    void testWindowTransiency();

protected Q_SLOTS:
    void onNewConnection(const QDBusConnection &connection);

private:
    RequestManager m_requestManager;
    Service m_service;
    QDBusConnection m_connection;
};

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
    parameters.insert(keyTimeout, 10);
    parameters.insert("hello", QString("world"));
    RequestReply *call = sendRequest(parameters);
    QSignalSpy callFinished(call, SIGNAL(finished()));

    QVERIFY(callFinished.wait());
    QCOMPARE(call->isError(), false);
    QCOMPARE(call->reply(), parameters);
    delete call;
}

void ServiceTest::testFailure()
{
    QString errorName("com.ubuntu.OnlineAccountsUi.BadLuck");
    QVariantMap parameters;
    parameters.insert(keyTimeout, 10);
    parameters.insert("fail", errorName);
    RequestReply *call = sendRequest(parameters);
    QSignalSpy callFinished(call, SIGNAL(finished()));

    QVERIFY(callFinished.wait());
    QCOMPARE(call->isError(), true);
    QCOMPARE(call->errorName(), errorName);
    delete call;
}

void ServiceTest::testIdle()
{
    QCOMPARE(m_requestManager.isIdle(), true);

    QSignalSpy isIdleChanged(&m_requestManager, SIGNAL(isIdleChanged()));

    QVariantMap parameters;
    parameters.insert(keyTimeout, 10);
    RequestReply *call = sendRequest(parameters);
    QSignalSpy callFinished(call, SIGNAL(finished()));

    QVERIFY(isIdleChanged.wait());
    QCOMPARE(m_requestManager.isIdle(), false);

    /* the request will terminate after 10 milliseconds, so expect the service
     * to be idle again */
    QVERIFY(isIdleChanged.wait());
    QCOMPARE(m_requestManager.isIdle(), true);

    QVERIFY(callFinished.wait());
    QCOMPARE(call->isError(), false);
    delete call;
}

void ServiceTest::testWindow()
{
    QVariantMap parameters;
    parameters.insert(keyTimeout, 10);
    RequestReply *call = sendRequest(parameters);
    QSignalSpy callFinished(call, SIGNAL(finished()));
    QSignalSpy windowShown(WindowWatcher::instance(),
                           SIGNAL(windowShown(QObject*)));

    QVERIFY(windowShown.wait());
    QWindow *window =
        qobject_cast<QWindow*>(windowShown.at(0).at(0).value<QObject*>());
    QVERIFY(window->property("transientParent").isNull());
    QVERIFY(callFinished.wait());
    QCOMPARE(call->isError(), false);
    delete call;

    QCOMPARE(windowShown.count(), 1);
}

void ServiceTest::testWindowTransiency()
{
    QVariantMap parameters;
    parameters.insert(keyTimeout, 10);
    parameters.insert(OAU_KEY_WINDOW_ID, 371);
    RequestReply *call = sendRequest(parameters);
    QSignalSpy callFinished(call, SIGNAL(finished()));
    QSignalSpy windowShown(WindowWatcher::instance(),
                           SIGNAL(windowShown(QObject*)));

    QVERIFY(windowShown.wait());
    QWindow *window =
        qobject_cast<QWindow*>(windowShown.at(0).at(0).value<QObject*>());
    QObject *transientParentObject =
        window->property("transientParent").value<QObject*>();
    QWindow *transientParent = qobject_cast<QWindow*>(transientParentObject);
    QCOMPARE(transientParent->property("winId").toUInt(), uint(371));
    QVERIFY(callFinished.wait());
    QCOMPARE(call->isError(), false);
    delete call;

    QCOMPARE(windowShown.count(), 1);
}

QTEST_MAIN(ServiceTest);

#include "tst_service.moc"

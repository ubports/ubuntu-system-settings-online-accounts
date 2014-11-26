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
#include "ipc.h"
#include "mock/request-mock.h"
#include "ui-proxy.h"

#include <QByteArray>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QLocalSocket>
#include <QMetaObject>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <SignOn/uisessiondata_priv.h>
#include <ubuntu-app-launch.h>

using namespace OnlineAccountsUi;

/* mocking the remote process { */
class RemoteProcess: public QObject {
    Q_OBJECT
public:
    RemoteProcess(const QString &program, const QString &appId,
                  const QStringList &arguments);
    ~RemoteProcess();

    Q_INVOKABLE bool run();
    void setResult(const QVariantMap &result);
    void fail(const QString &errorName, const QString &errorMessage);
    void registerHandler(const QString &matchId);

    const QVariantMap &lastReceived() const { return m_lastData; }
    QString programName() const { return m_program; }
    QString appId() const { return m_appId; }
    QStringList arguments() const { return m_arguments; }
    QByteArray instanceId() const { return m_instanceId; }
    void sendOperation(const QVariantMap &data);

private Q_SLOTS:
    void onDataReady(QByteArray &data);

Q_SIGNALS:
    void dataReceived(QVariantMap data);

private:
    QString m_program;
    QString m_appId;
    QStringList m_arguments;
    QByteArray m_instanceId;
    QVariantMap m_lastData;
    int m_requestId;
    QString m_requestInterface;
    QLocalSocket m_socket;
    Ipc m_ipc;
};

static QMap<QByteArray, RemoteProcess *> remoteProcesses;
static int processInstance = 1;

RemoteProcess::RemoteProcess(const QString &program, const QString &appId,
                             const QStringList &arguments):
    QObject(),
    m_program(program),
    m_appId(appId),
    m_arguments(arguments),
    m_instanceId(QByteArray::number(processInstance++))
{
    QObject::connect(&m_ipc, SIGNAL(dataReady(QByteArray &)),
                     this, SLOT(onDataReady(QByteArray &)));
    QObject::connect(&m_socket, SIGNAL(disconnected()),
                     this, SLOT(deleteLater()));
}

RemoteProcess::~RemoteProcess()
{
    remoteProcesses.remove(m_instanceId);
}

bool RemoteProcess::run()
{
    m_socket.connectToServer(m_arguments[0]);
    if (Q_UNLIKELY(!m_socket.waitForConnected())) return false;

    m_ipc.setChannels(&m_socket, &m_socket);
    return true;
}

void RemoteProcess::setResult(const QVariantMap &result)
{
    QVariantMap operation;
    operation.insert(OAU_OPERATION_CODE,
                     OAU_OPERATION_CODE_REQUEST_FINISHED);
    operation.insert(OAU_OPERATION_ID, m_requestId);
    operation.insert(OAU_OPERATION_INTERFACE, m_requestInterface);
    operation.insert(OAU_OPERATION_DATA, result);
    sendOperation(operation);
    deleteLater();
}

void RemoteProcess::fail(const QString &errorName, const QString &errorMessage)
{
    QVariantMap operation;
    operation.insert(OAU_OPERATION_CODE,
                     OAU_OPERATION_CODE_REQUEST_FAILED);
    operation.insert(OAU_OPERATION_ID, m_requestId);
    operation.insert(OAU_OPERATION_INTERFACE, m_requestInterface);
    operation.insert(OAU_OPERATION_ERROR_NAME, errorName);
    operation.insert(OAU_OPERATION_ERROR_MESSAGE, errorMessage);
    sendOperation(operation);
    deleteLater();
}

void RemoteProcess::registerHandler(const QString &matchId)
{
    QVariantMap operation;
    operation.insert(OAU_OPERATION_CODE,
                     OAU_OPERATION_CODE_REGISTER_HANDLER);
    operation.insert(OAU_OPERATION_HANDLER_ID, matchId);
    sendOperation(operation);
}

void RemoteProcess::sendOperation(const QVariantMap &data)
{
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << data;
    m_ipc.write(ba);
}

void RemoteProcess::onDataReady(QByteArray &data)
{
    QVariantMap map;
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream >> map;

    m_lastData = map;
    if (map[OAU_OPERATION_CODE].toString() == OAU_OPERATION_CODE_PROCESS) {
        m_requestInterface = map[OAU_OPERATION_INTERFACE].toString();
        m_requestId = map[OAU_OPERATION_ID].toInt();
    }
    Q_EMIT dataReceived(map);
}
/* } mocking the remote process */

/* mocking ubuntu-app-launch-2 { */
gchar *
ubuntu_app_launch_start_multiple_helper(const gchar *type,
                                        const gchar *appid,
                                        const gchar * const *uris)
{
    QStringList arguments;
    for (int i = 0; uris[i] != NULL; i++) {
        arguments.append(QString::fromUtf8(uris[i]));
    }
    RemoteProcess *process = new RemoteProcess(QString::fromUtf8(type),
                                               QString::fromUtf8(appid),
                                               arguments);
    remoteProcesses.insert(process->instanceId(), process);
    QMetaObject::invokeMethod(process, "run", Qt::QueuedConnection);
    return g_strdup(process->instanceId().constData());
}
/* } mocking ubuntu-app-launch-2 */

class UiProxyTest: public QObject
{
    Q_OBJECT

public:
    UiProxyTest();

private:
    Request *createRequest(const QString &interface,
                           const QString &method,
                           const QString &clientApparmorProfile,
                           const QVariantMap &parameters);

private Q_SLOTS:
    void testInit();
    void testRequest_data();
    void testRequest();
    void testHandler();

private:
    QDBusConnection m_connection;
};

UiProxyTest::UiProxyTest():
    QObject(0),
    m_connection(QStringLiteral("uninitialized"))
{
}

Request *UiProxyTest::createRequest(const QString &interface,
                                    const QString &method,
                                    const QString &clientApparmorProfile,
                                    const QVariantMap &parameters)
{
    QDBusMessage message =
        QDBusMessage::createMethodCall(OAU_SERVICE_NAME,
                                       OAU_OBJECT_PATH,
                                       interface,
                                       method);
    Request *request = new Request(m_connection, message, parameters);
    RequestPrivate *r = RequestPrivate::mocked(request);

    r->setClientApparmorProfile(clientApparmorProfile);
    return request;
}

void UiProxyTest::testInit()
{
    /* By passing pid = 0, we disable the prompt session code */
    UiProxy *proxy = new UiProxy(0, this);
    QVERIFY(proxy->init());
    delete proxy;

    proxy = new UiProxy(10, this);
    QVERIFY(proxy->init());
    delete proxy;
}

void UiProxyTest::testRequest_data()
{
    QTest::addColumn<QString>("interface");
    QTest::addColumn<QString>("method");
    QTest::addColumn<QString>("clientApparmorProfile");
    QTest::addColumn<QVariantMap>("parameters");
    QTest::addColumn<QVariantMap>("expectedResult");
    QTest::addColumn<QString>("expectedError");

    QVariantMap parameters;
    QVariantMap result;

    parameters.insert("greeting", "Hello!");
    result.insert("response", "How do you do?");
    QTest::newRow("success") <<
        OAU_INTERFACE <<
        "doSomething" <<
        "com.ubuntu.package_app_0.1" <<
        parameters <<
        result << "";
    parameters.clear();
    result.clear();

    parameters.insert("greeting", "Hi!");
    QTest::newRow("failure") <<
        OAU_INTERFACE <<
        "doSomethingElse" <<
        "com.ubuntu.package_app_0.2" <<
        parameters <<
        result << "Error code";
    parameters.clear();
}

void UiProxyTest::testRequest()
{
    QFETCH(QString, interface);
    QFETCH(QString, method);
    QFETCH(QString, clientApparmorProfile);
    QFETCH(QVariantMap, parameters);
    QFETCH(QVariantMap, expectedResult);
    QFETCH(QString, expectedError);

    Request *request = createRequest(interface, method,
                                     clientApparmorProfile, parameters);
    RequestPrivate *r = RequestPrivate::mocked(request);
    QSignalSpy requestFailCalled(r, SIGNAL(failCalled(QString,QString)));
    QSignalSpy requestSetResultCalled(r, SIGNAL(setResultCalled(QVariantMap)));

    UiProxy *proxy = new UiProxy(0, this);
    QVERIFY(proxy->init());

    QSignalSpy finished(proxy, SIGNAL(finished()));
    proxy->handleRequest(request);

    QTRY_VERIFY(!remoteProcesses.isEmpty());
    QCOMPARE(remoteProcesses.count(), 1);

    RemoteProcess *process = remoteProcesses.values().first();
    QVERIFY(process);
    QSignalSpy dataReceived(process, SIGNAL(dataReceived(QVariantMap)));
    QCOMPARE(process->programName(), QString("online-accounts-ui"));
    QCOMPARE(process->appId(), QString("unconfined"));

    /* Check the received data */
    if (process->lastReceived().isEmpty()) {
        QVERIFY(dataReceived.wait());
    }

    QVariantMap data = process->lastReceived();
    QCOMPARE(data.value(OAU_OPERATION_CODE).toString(),
             QStringLiteral(OAU_OPERATION_CODE_PROCESS));
    QVERIFY(data.contains(OAU_OPERATION_ID));
    QCOMPARE(data.value(OAU_OPERATION_DATA).toMap(), parameters);
    QCOMPARE(data.value(OAU_OPERATION_INTERFACE).toString(), interface);
    QCOMPARE(data.value(OAU_OPERATION_CLIENT_PROFILE).toString(),
             clientApparmorProfile);

    if (expectedError.isEmpty()) {
        process->setResult(expectedResult);
        QVERIFY(requestSetResultCalled.wait());
        QCOMPARE(requestSetResultCalled.count(), 1);
        QCOMPARE(requestSetResultCalled.at(0).at(0).toMap(), expectedResult);
    } else {
        process->fail(expectedError, "some error");
        QVERIFY(requestFailCalled.wait());
        QCOMPARE(requestFailCalled.count(), 1);
        QCOMPARE(requestFailCalled.at(0).at(0).toString(), expectedError);
    }

    if (finished.count() == 0) {
        QVERIFY(finished.wait());
    }
    QCOMPARE(finished.count(), 1);

    delete proxy;
}

void UiProxyTest::testHandler()
{
    UiProxy *proxy = new UiProxy(0, this);
    QVERIFY(proxy->init());

    /* First, try with empty match parameters */
    QVariantMap matchParams;
    QVERIFY(!proxy->hasHandlerFor(matchParams));

    /* Valid parameters, but without having registered a handler */
    QString match("something unique");
    QVariantMap clientData;
    clientData.insert(OAU_REQUEST_MATCH_KEY, match);
    matchParams.insert(SSOUI_KEY_CLIENT_DATA, clientData);
    QVERIFY(!proxy->hasHandlerFor(matchParams));

    /* Then, really register a handler */
    QVariantMap parameters;
    parameters.insert("greeting", "hi!");
    Request *request = createRequest("iface", "doSomething",
                                     "com.ubuntu.package_app_0.1", parameters);
    proxy->handleRequest(request);

    QTRY_VERIFY(!remoteProcesses.isEmpty());
    QCOMPARE(remoteProcesses.count(), 1);

    RemoteProcess *process = remoteProcesses.values().first();
    QVERIFY(process);
    QSignalSpy dataReceived(process, SIGNAL(dataReceived(QVariantMap)));

    /* Wait for the request to arrive */
    if (process->lastReceived().isEmpty()) {
        QVERIFY(dataReceived.wait());
    }

    /* Register a handler */
    process->registerHandler(match);
    QTRY_VERIFY(proxy->hasHandlerFor(matchParams));

    /* make sure that a different key doesn't match */
    clientData.insert(OAU_REQUEST_MATCH_KEY, QString("Won't match"));
    matchParams.insert(SSOUI_KEY_CLIENT_DATA, clientData);
    QVERIFY(!proxy->hasHandlerFor(matchParams));

    delete proxy;
}

QTEST_MAIN(UiProxyTest);

#include "tst_ui_proxy.moc"

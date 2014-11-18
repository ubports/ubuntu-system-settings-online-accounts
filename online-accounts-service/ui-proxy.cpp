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
#include "ipc.h"
#include "mir-helper.h"
#include "request.h"
#include "ui-proxy.h"

#include <Accounts/Manager>
#include <Accounts/Provider>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <SignOn/uisessiondata_priv.h>
#include <ubuntu-app-launch.h>

using namespace OnlineAccountsUi;

static int socketCounter = 1;

namespace OnlineAccountsUi {

class UiProxyPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(UiProxy)

public:
    inline UiProxyPrivate(pid_t clientPid, UiProxy *pluginProxy);
    inline ~UiProxyPrivate();

    void setStatus(UiProxy::Status status);
    bool setupSocket();
    bool init();
    void sendOperation(const QVariantMap &data);
    void sendRequest(int requestId, Request *request);
    QString setupPromptSession();
    QString findAppArmorProfile();
    void startProcess();

private Q_SLOTS:
    void onNewConnection();
    void onDataReady(QByteArray &data);
    void onRequestCompleted();

private:
    UiProxy::Status m_status;
    QLocalServer m_server;
    QLocalSocket *m_socket;
    OnlineAccountsUi::Ipc m_ipc;
    int m_nextRequestId;
    QMap<int,Request*> m_requests;
    QStringList m_handlers;
    pid_t m_clientPid;
    PromptSession *m_promptSession;
    QList<QByteArray> m_arguments;
    mutable UiProxy *q_ptr;
};

} // namespace

UiProxyPrivate::UiProxyPrivate(pid_t clientPid, UiProxy *uiProxy):
    QObject(uiProxy),
    m_status(UiProxy::Null),
    m_socket(0),
    m_nextRequestId(0),
    m_clientPid(clientPid),
    m_promptSession(0),
    q_ptr(uiProxy)
{
    QObject::connect(&m_server, SIGNAL(newConnection()),
                     this, SLOT(onNewConnection()));
    QObject::connect(&m_ipc, SIGNAL(dataReady(QByteArray &)),
                     this, SLOT(onDataReady(QByteArray &)));
}

UiProxyPrivate::~UiProxyPrivate()
{
    /* Cancel all requests */
    Q_FOREACH(Request *request, m_requests) {
        request->cancel();
    }

    delete m_promptSession;

    if (m_socket) {
        m_socket->abort();
        delete m_socket;
    }
    m_server.close();
}

void UiProxyPrivate::setStatus(UiProxy::Status status)
{
    Q_Q(UiProxy);
    if (m_status == status) return;
    m_status = status;
    Q_EMIT q->statusChanged();
}

void UiProxyPrivate::sendOperation(const QVariantMap &data)
{
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << data;
    m_ipc.write(ba);
}

void UiProxyPrivate::onNewConnection()
{
    Q_Q(UiProxy);

    QLocalSocket *socket = m_server.nextPendingConnection();
    if (Q_UNLIKELY(socket == 0)) return;

    if (Q_UNLIKELY(m_socket != 0)) {
        qWarning() << "A socket is already active!";
        socket->deleteLater();
        return;
    }

    m_socket = socket;
    QObject::connect(socket, SIGNAL(disconnected()),
                     q, SIGNAL(finished()));
    m_ipc.setChannels(socket, socket);
    m_server.close(); // stop listening

    setStatus(UiProxy::Ready);

    /* Execute any pending requests */
    QMapIterator<int, Request*> it(m_requests);
    while (it.hasNext()) {
        it.next();
        sendRequest(it.key(), it.value());
    }
}

void UiProxyPrivate::onDataReady(QByteArray &data)
{
    QVariantMap map;
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream >> map;

    DEBUG() << map;

    int requestId = map.value(OAU_OPERATION_ID, -1).toInt();
    Request *request = m_requests.value(requestId, 0);

    QString code = map.value(OAU_OPERATION_CODE).toString();
    if (code == OAU_OPERATION_CODE_REQUEST_FINISHED) {
        Q_ASSERT(request);
        request->setResult(map.value(OAU_OPERATION_DATA).toMap());
    } else if (code == OAU_OPERATION_CODE_REQUEST_FAILED) {
        Q_ASSERT(request);
        request->fail(map.value(OAU_OPERATION_ERROR_NAME).toString(),
                      map.value(OAU_OPERATION_ERROR_MESSAGE).toString());
    } else if (code == OAU_OPERATION_CODE_REGISTER_HANDLER) {
        m_handlers.append(map.value(OAU_OPERATION_HANDLER_ID).toString());
    } else {
        qWarning() << "Invalid operation code: " << code;
    }
}

bool UiProxyPrivate::setupSocket()
{
    QString runtimeDir =
        QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QDir socketDir(runtimeDir + "/online-accounts-ui");
    if (!socketDir.exists()) socketDir.mkpath(".");

    QString uniqueName = QString("ui-%1").arg(socketCounter++);

    /* If the file exists, it's a stale file: online-accounts-ui is a single
     * instance process, and the only one creating files in this directory. */
    if (Q_UNLIKELY(socketDir.exists(uniqueName))) {
        socketDir.remove(uniqueName);
    }

    /* Create the socket and set it into listen mode */
    return m_server.listen(socketDir.filePath(uniqueName));
}

QString UiProxyPrivate::setupPromptSession()
{
    Q_Q(UiProxy);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!env.value("QT_QPA_PLATFORM").startsWith("ubuntu")) return QString();

    PromptSession *session =
        MirHelper::instance()->createPromptSession(m_clientPid);
    if (!session) return QString();

    QString mirSocket = session->requestSocket();
    if (mirSocket.isEmpty()) {
        delete session;
        return mirSocket;
    }

    m_promptSession = session;
    QObject::connect(m_promptSession, SIGNAL(finished()),
                     q, SIGNAL(finished()));

    return mirSocket;
}

bool UiProxyPrivate::init()
{
    if (Q_UNLIKELY(!setupSocket())) return false;

    m_arguments.clear();
    m_arguments.append(m_server.fullServerName().toUtf8());

    QByteArray mirSocket;
    if (m_clientPid) {
        mirSocket = setupPromptSession().toUtf8();
    }
    if (mirSocket.isEmpty()) {
        mirSocket = "invalid://url";
    }
    m_arguments.append(mirSocket);
    return true;
}

QString UiProxyPrivate::findAppArmorProfile()
{
    QString providerId;
    /* Look through the requests, and look for the first one which has a
     * provider set. We'll use that provider's AppArmor id for confinement. */
    Q_FOREACH(Request *request, m_requests) {
        providerId = request->providerId();
        if (!providerId.isEmpty())
            break;
    }

    if (Q_UNLIKELY(providerId.isEmpty())) return QString();

    /* Load the provider XML file */
    Accounts::Manager manager;
    Accounts::Provider provider = manager.provider(providerId);
    if (Q_UNLIKELY(!provider.isValid())) {
        qWarning() << "Provider not found:" << providerId;
        return QString();
    }

    const QDomDocument doc = provider.domDocument();
    QDomElement root = doc.documentElement();
    return root.firstChildElement("profile").text();
}

void UiProxyPrivate::startProcess()
{
    QVector<const gchar*> uris;
    Q_FOREACH(const QByteArray &arg, m_arguments) {
        uris.append(arg.constData());
    }
    uris.append(NULL);

    QString profile = findAppArmorProfile();
    if (profile.isEmpty()) {
        profile = "unconfined";
    }

    gchar *instanceId =
        ubuntu_app_launch_start_multiple_helper("online-accounts-ui",
                                                profile.toUtf8().constData(),
                                                uris.constData());
    if (Q_UNLIKELY(instanceId == NULL)) {
        qWarning() << "Couldn't start account plugin process";
        setStatus(UiProxy::Error);
        return;
    }

    setStatus(UiProxy::Loading);
}

void UiProxyPrivate::sendRequest(int requestId, Request *request)
{
    QVariantMap operation;
    operation.insert(OAU_OPERATION_CODE, OAU_OPERATION_CODE_PROCESS);
    operation.insert(OAU_OPERATION_ID, requestId);
    operation.insert(OAU_OPERATION_DATA, request->parameters());
    operation.insert(OAU_OPERATION_INTERFACE, request->interface());
    operation.insert(OAU_OPERATION_CLIENT_PROFILE,
                     request->clientApparmorProfile());
    sendOperation(operation);
}

void UiProxyPrivate::onRequestCompleted()
{
    Q_Q(UiProxy);

    Request *request = qobject_cast<Request*>(sender());
    int id = m_requests.key(request, -1);
    if (id != -1) {
        m_requests.remove(id);
        if (m_requests.isEmpty()) {
            Q_EMIT q->finished();
        }
    }
}

UiProxy::UiProxy(pid_t clientPid, QObject *parent):
    QObject(parent),
    d_ptr(new UiProxyPrivate(clientPid, this))
{
}

UiProxy::~UiProxy()
{
}

UiProxy::Status UiProxy::status() const
{
    Q_D(const UiProxy);
    return d->m_status;
}

bool UiProxy::init()
{
    Q_D(UiProxy);
    return d->init();
}

void UiProxy::handleRequest(Request *request)
{
    Q_D(UiProxy);

    int requestId = d->m_nextRequestId++;
    d->m_requests.insert(requestId, request);
    QObject::connect(request, SIGNAL(completed()),
                     d, SLOT(onRequestCompleted()));
    request->setInProgress(true);

    if (d->m_status == UiProxy::Ready) {
        d->sendRequest(requestId, request);
    } else if (d->m_status == UiProxy::Null) {
        d->startProcess();
    }
}

bool UiProxy::hasHandlerFor(const QVariantMap &parameters)
{
    Q_D(UiProxy);

    /* Find if there's any handlers expecting to handle the SignOnUi
     * request having "parameters" as parameters.
     * This is simply done by matching on the X-RequestHandler key (aka
     * matchKey()), if present. We expect that account plugins add that field
     * to their AuthSession requests which they want to handle themselves.
     */
    DEBUG() << parameters;
    if (!parameters.contains(SSOUI_KEY_CLIENT_DATA)) return false;
    QVariant variant = parameters[SSOUI_KEY_CLIENT_DATA];
    QVariantMap clientData = variant.toMap();
    QString matchId = clientData.value(OAU_REQUEST_MATCH_KEY).toString();
    if (matchId.isEmpty()) return false;

    return d->m_handlers.contains(matchId);
}

#include "ui-proxy.moc"

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
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>
#include <SignOn/uisessiondata_priv.h>

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
    bool setupPromptSession();
    QString findAppArmorProfile();
    void startProcess();

private Q_SLOTS:
    void onNewConnection();
    void onDisconnected();
    void onDataReady(QByteArray &data);
    void onRequestCompleted();
    void onFinishedTimer();

private:
    QProcess m_process;
    UiProxy::Status m_status;
    QLocalServer m_server;
    QLocalSocket *m_socket;
    OnlineAccountsUi::Ipc m_ipc;
    QTimer m_finishedTimer;
    int m_nextRequestId;
    QMap<int,Request*> m_requests;
    QStringList m_handlers;
    pid_t m_clientPid;
    QString m_providerId;
    PromptSessionP m_promptSession;
    QStringList m_arguments;
    mutable UiProxy *q_ptr;
};

} // namespace

UiProxyPrivate::UiProxyPrivate(pid_t clientPid, UiProxy *uiProxy):
    QObject(uiProxy),
    m_status(UiProxy::Null),
    m_socket(0),
    m_nextRequestId(0),
    m_clientPid(clientPid),
    q_ptr(uiProxy)
{
    QObject::connect(&m_server, SIGNAL(newConnection()),
                     this, SLOT(onNewConnection()));
    QObject::connect(&m_ipc, SIGNAL(dataReady(QByteArray &)),
                     this, SLOT(onDataReady(QByteArray &)));
    m_process.setProcessChannelMode(QProcess::ForwardedChannels);

    m_finishedTimer.setSingleShot(true);
    QObject::connect(&m_finishedTimer, SIGNAL(timeout()),
                     this, SLOT(onFinishedTimer()));
}

UiProxyPrivate::~UiProxyPrivate()
{
    /* Cancel all requests */
    Q_FOREACH(Request *request, m_requests) {
        request->cancel();
    }

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

void UiProxyPrivate::onDisconnected()
{
    Q_Q(UiProxy);

    if (!m_finishedTimer.isActive()) {
        Q_EMIT q->finished();
    }
}

void UiProxyPrivate::onNewConnection()
{
    QLocalSocket *socket = m_server.nextPendingConnection();
    if (Q_UNLIKELY(socket == 0)) return;

    if (Q_UNLIKELY(m_socket != 0)) {
        qWarning() << "A socket is already active!";
        socket->deleteLater();
        return;
    }

    m_socket = socket;
    QObject::connect(socket, SIGNAL(disconnected()),
                     this, SLOT(onDisconnected()));
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
        request->setDelay(map.value(OAU_OPERATION_DELAY).toInt());
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

    QString uniqueName = QString("ui-%1-%2").arg(socketCounter++).arg(m_providerId);

    /* If the file exists, it's a stale file: online-accounts-ui is a single
     * instance process, and the only one creating files in this directory. */
    if (Q_UNLIKELY(socketDir.exists(uniqueName))) {
        socketDir.remove(uniqueName);
    }

    /* Create the socket and set it into listen mode */
    return m_server.listen(socketDir.filePath(uniqueName));
}

bool UiProxyPrivate::setupPromptSession()
{
    Q_Q(UiProxy);

    if (!m_clientPid) return false;

    PromptSessionP session =
        MirHelper::instance()->createPromptSession(m_clientPid);
    if (!session) return false;

    QString mirSocket = session->requestSocket();
    if (mirSocket.isEmpty()) {
        return false;
    }

    m_promptSession = session;
    QObject::connect(m_promptSession.data(), SIGNAL(finished()),
                     q, SIGNAL(finished()));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("MIR_SOCKET", mirSocket);
    m_process.setProcessEnvironment(env);
    return true;
}

bool UiProxyPrivate::init()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.value("QT_QPA_PLATFORM").startsWith("ubuntu")) {
        if (!setupPromptSession()) {
            qWarning() << "Couldn't setup prompt session";
            return false;
        }
    } else {
        env.insert("DESKTOP_FILE_HINT", "online-accounts-ui");
        m_process.setProcessEnvironment(env);
    }

    /* We also create ~/cache/online-accounts-ui/, since the plugin might not
     * have permissions to do that. */
    QString userCacheDir =
        QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    QDir cacheDir(userCacheDir + "/online-accounts-ui");
    if (!cacheDir.exists()) cacheDir.mkpath(".");

    return true;
}

QString UiProxyPrivate::findAppArmorProfile()
{
    if (Q_UNLIKELY(m_providerId.isEmpty())) return QString();

    /* Load the provider XML file */
    Accounts::Manager manager;
    Accounts::Provider provider = manager.provider(m_providerId);
    if (Q_UNLIKELY(!provider.isValid())) {
        qWarning() << "Provider not found:" << m_providerId;
        return QString();
    }

    const QDomDocument doc = provider.domDocument();
    QDomElement root = doc.documentElement();
    return root.firstChildElement("profile").text();
}

void UiProxyPrivate::startProcess()
{
    if (Q_UNLIKELY(!setupSocket())) {
        qWarning() << "Couldn't setup IPC socket";
        setStatus(UiProxy::Error);
        return;
    }
    m_arguments.append("--socket");
    m_arguments.append(m_server.fullServerName());

    QString profile = findAppArmorProfile();
    if (profile.isEmpty()) {
        profile = "unconfined";
    } else {
        QProcessEnvironment env = m_process.processEnvironment();
        env.insert("APP_ID", profile);
        /* Set TMPDIR to a location which the confined process can actually
         * use */
        QString tmpdir =
            QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) +
            "/" + profile.split('_')[0];
        env.insert("TMPDIR", tmpdir);
        m_process.setProcessEnvironment(env);
    }

    m_arguments.append("--profile");
    m_arguments.append(profile);

    QString processName;
    QString wrapper = QString::fromUtf8(qgetenv("OAU_WRAPPER"));
    QString accountsUi = QStringLiteral(INSTALL_BIN_DIR "/online-accounts-ui");
    if (wrapper.isEmpty()) {
        processName = accountsUi;
    } else {
        processName = wrapper;
        m_arguments.prepend(accountsUi);
    }

    setStatus(UiProxy::Loading);
    m_process.start(processName, m_arguments);
    if (Q_UNLIKELY(!m_process.waitForStarted())) {
        qWarning() << "Couldn't start account plugin process";
        setStatus(UiProxy::Error);
        return;
    }
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

void UiProxyPrivate::onFinishedTimer()
{
    Q_Q(UiProxy);

    if (m_requests.isEmpty()) {
        Q_EMIT q->finished();
    }
}

void UiProxyPrivate::onRequestCompleted()
{
    Request *request = qobject_cast<Request*>(sender());
    Q_ASSERT(request);
    m_finishedTimer.setInterval(request->delay());
    m_finishedTimer.start();

    int id = m_requests.key(request, -1);
    if (id != -1) {
        m_requests.remove(id);
    }
}

UiProxy::UiProxy(pid_t clientPid, QObject *parent):
    QObject(parent),
    d_ptr(new UiProxyPrivate(clientPid, this))
{
}

UiProxy::~UiProxy()
{
    DEBUG();
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

    if (d->m_providerId.isEmpty()) {
        d->m_providerId = request->providerId();
    }
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

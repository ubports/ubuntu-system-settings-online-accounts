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
#include "request.h"
#include "signonui-request.h"
#include "ui-server.h"

#include <OnlineAccountsPlugin/request-handler.h>
#include <QLocalSocket>
#include <QtQml>
#include <SignOn/uisessiondata_priv.h>

using namespace OnlineAccountsUi;

static UiServer *m_instance = 0;

namespace OnlineAccountsUi {

class UiServerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(UiServer)

public:
    inline UiServerPrivate(const QString &address, UiServer *pluginServer);
    inline ~UiServerPrivate();

    bool setupSocket();
    bool init();
    void sendOperation(const QVariantMap &data);

private Q_SLOTS:
    void onDataReady(QByteArray &data);
    void onRequestCompleted();
    void registerHandler(SignOnUi::RequestHandler *handler);

private:
    QLocalSocket m_socket;
    OnlineAccountsUi::Ipc m_ipc;
    SignOnUi::RequestHandlerWatcher m_handlerWatcher;
    mutable UiServer *q_ptr;
};

} // namespace

UiServerPrivate::UiServerPrivate(const QString &address,
                                 UiServer *pluginServer):
    QObject(pluginServer),
    q_ptr(pluginServer)
{
    QObject::connect(&m_ipc, SIGNAL(dataReady(QByteArray &)),
                     this, SLOT(onDataReady(QByteArray &)));
    QObject::connect(&m_socket, SIGNAL(disconnected()),
                     q_ptr, SIGNAL(finished()));
    m_socket.connectToServer(address);

    QObject::connect(&m_handlerWatcher,
                     SIGNAL(newHandler(SignOnUi::RequestHandler *)),
                     this,
                     SLOT(registerHandler(SignOnUi::RequestHandler *)));
}

UiServerPrivate::~UiServerPrivate()
{
    DEBUG();
}

void UiServerPrivate::sendOperation(const QVariantMap &data)
{
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << data;
    m_ipc.write(ba);
}

void UiServerPrivate::onDataReady(QByteArray &data)
{
    QVariantMap map;
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream >> map;

    DEBUG() << map;

    QString code = map.value(OAU_OPERATION_CODE).toString();
    if (code == OAU_OPERATION_CODE_PROCESS) {
        QVariantMap parameters = map[OAU_OPERATION_DATA].toMap();
        Request *request =
            Request::newRequest(map[OAU_OPERATION_INTERFACE].toString(),
                                map[OAU_OPERATION_ID].toInt(),
                                map[OAU_OPERATION_CLIENT_PROFILE].toString(),
                                parameters,
                                this);
        QObject::connect(request, SIGNAL(completed()),
                         this, SLOT(onRequestCompleted()));

        /* Check if a RequestHandler has been setup to handle this request. If
         * so, bing the request object to the handler and start the request
         * immediately. */
        SignOnUi::Request *signonRequest =
            qobject_cast<SignOnUi::Request*>(request);
        if (signonRequest) {
            SignOnUi::RequestHandler *handler =
                m_handlerWatcher.findMatching(parameters);
            if (handler) {
                signonRequest->setHandler(handler);
            }
        }
        request->start();
    } else {
        qWarning() << "Invalid operation code: " << code;
    }
}

void UiServerPrivate::onRequestCompleted()
{
    Request *request = qobject_cast<Request*>(sender());
    request->disconnect(this);
    request->deleteLater();

    if (request->errorName().isEmpty()) {
        QVariantMap operation;
        operation.insert(OAU_OPERATION_CODE,
                         OAU_OPERATION_CODE_REQUEST_FINISHED);
        operation.insert(OAU_OPERATION_ID, request->id());
        operation.insert(OAU_OPERATION_DATA, request->result());
        operation.insert(OAU_OPERATION_DELAY, request->delay());
        operation.insert(OAU_OPERATION_INTERFACE, request->interface());
        sendOperation(operation);
    } else {
        QVariantMap operation;
        operation.insert(OAU_OPERATION_CODE,
                         OAU_OPERATION_CODE_REQUEST_FAILED);
        operation.insert(OAU_OPERATION_ID, request->id());
        operation.insert(OAU_OPERATION_INTERFACE, request->interface());
        operation.insert(OAU_OPERATION_ERROR_NAME, request->errorName());
        operation.insert(OAU_OPERATION_ERROR_MESSAGE, request->errorMessage());
        sendOperation(operation);
    }
}

bool UiServerPrivate::init()
{
    if (Q_UNLIKELY(!m_socket.waitForConnected())) return false;

    m_ipc.setChannels(&m_socket, &m_socket);
    return true;
}

void UiServerPrivate::registerHandler(SignOnUi::RequestHandler *handler)
{
    QVariantMap operation;
    operation.insert(OAU_OPERATION_CODE,
                     OAU_OPERATION_CODE_REGISTER_HANDLER);
    operation.insert(OAU_OPERATION_HANDLER_ID, handler->matchId());
    sendOperation(operation);
}

UiServer::UiServer(const QString &address, QObject *parent):
    QObject(parent),
    d_ptr(new UiServerPrivate(address, this))
{
    m_instance = this;
}

UiServer::~UiServer()
{
    m_instance = 0;
}

UiServer *UiServer::instance()
{
    return m_instance;
}

bool UiServer::init()
{
    Q_D(UiServer);
    return d->init();
}

#include "ui-server.moc"

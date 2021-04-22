/*
 * This file is part of libAuthentication
 *
 * Copyright (C) 2017-2020 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "loopback-server.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>

using namespace OnlineAccountsPlugin;

namespace OnlineAccountsPlugin {

class LoopbackServerPrivate: public QTcpServer
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(LoopbackServer)

public:
    LoopbackServerPrivate(LoopbackServer *q);
    ~LoopbackServerPrivate();

    bool listen(uint16_t port);
    void respond(QTcpSocket *socket);

private Q_SLOTS:
    void onNewConnection();
    void onReadyRead();

private:
    int m_portIncrementAttempts;
    QByteArray m_data;
    QByteArray m_servedHtml;
    LoopbackServer *q_ptr;
};

} // namespace

LoopbackServerPrivate::LoopbackServerPrivate(LoopbackServer *q):
    m_portIncrementAttempts(0),
    q_ptr(q)
{
    QObject::connect(this, &QTcpServer::newConnection,
                     this, &LoopbackServerPrivate::onNewConnection);

    m_servedHtml =
        QByteArrayLiteral("<html><head><title>") +
        qApp->applicationName().toUtf8() +
        QByteArrayLiteral("</title></head><body>") +
        QObject::tr("Authentication completed; you can close this page "
                    "and go back to the application.").
        toUtf8() +
        QByteArrayLiteral("</body></html>");
}

LoopbackServerPrivate::~LoopbackServerPrivate()
{
}

bool LoopbackServerPrivate::listen(uint16_t port)
{
    uint16_t attempts = 0;

    do {
        bool ok = QTcpServer::listen(QHostAddress::Any,
                                     quint16(port + attempts));
        if (ok) return true;
    } while (attempts++ < m_portIncrementAttempts);

    return false;
}

void LoopbackServerPrivate::respond(QTcpSocket *socket)
{
    QByteArray htmlSize = QString::number(m_servedHtml.size()).toUtf8();
    QByteArray replyMessage =
        QByteArrayLiteral("HTTP/1.0 200 OK \r\n"
                          "Content-Type: text/html; charset=\"utf-8\"\r\n"
                          "Content-Length: ") + htmlSize +
        QByteArrayLiteral("\r\n\r\n") +
        m_servedHtml;

    socket->write(replyMessage);
}

void LoopbackServerPrivate::onNewConnection()
{
    QTcpSocket *socket = nextPendingConnection();
    Q_ASSERT(socket);
    QObject::connect(socket, &QTcpSocket::disconnected,
                     socket, &QTcpSocket::deleteLater);
    QObject::connect(socket, &QTcpSocket::readyRead,
                     this, &LoopbackServerPrivate::onReadyRead);
}

void LoopbackServerPrivate::onReadyRead()
{
    Q_Q(LoopbackServer);

    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    Q_ASSERT(socket);

    m_data += socket->readAll();
    int lineLength = m_data.indexOf('\r');
    if (lineLength < 0) {
        /* Not all data has been received; will get more later */
        return;
    }
    QByteArray firstLine = m_data.left(lineLength);
    QList<QByteArray> parts = firstLine.split(' ');
    if (parts.count() >= 2) {
        QString path = QString::fromUtf8(parts[1]);
        QUrl url(QString::fromLatin1("http://localhost:%1%2").
                 arg(serverPort()).arg(path));
        Q_EMIT q->visited(url);
    }

    respond(socket);
    socket->disconnectFromHost();
}

LoopbackServer::LoopbackServer(QObject *parent):
    QObject(parent),
    d_ptr(new LoopbackServerPrivate(this))
{
}

LoopbackServer::~LoopbackServer()
{
}

bool LoopbackServer::listen(uint16_t port)
{
    Q_D(LoopbackServer);

    bool ok = d->listen(port);

    Q_EMIT callbackUrlChanged();

    return ok;
}

void LoopbackServer::close()
{
    Q_D(LoopbackServer);
    d->close();

    Q_EMIT callbackUrlChanged();
}

uint16_t LoopbackServer::port() const
{
    Q_D(const LoopbackServer);
    return d->serverPort();
}

void LoopbackServer::setPortIncrementAttempts(int maxAttempts)
{
    Q_D(LoopbackServer);
    if (d->m_portIncrementAttempts == maxAttempts) return;
    d->m_portIncrementAttempts = maxAttempts;
    Q_EMIT portIncrementAttemptsChanged();
}

int LoopbackServer::portIncrementAttempts() const
{
    Q_D(const LoopbackServer);
    return d->m_portIncrementAttempts;
}

void LoopbackServer::setServedHtml(const QByteArray &html)
{
    Q_D(LoopbackServer);
    d->m_servedHtml = html;
    Q_EMIT servedHtmlChanged();
}

QByteArray LoopbackServer::servedHtml() const
{
    Q_D(const LoopbackServer);
    return d->m_servedHtml;
}

QUrl LoopbackServer::callbackUrl() const
{
    Q_D(const LoopbackServer);
    if (!d->isListening())
        return QUrl();
    return QUrl(QString::fromLatin1("http://localhost:%1/").
                arg(d->serverPort()));
}

#include "loopback-server.moc"

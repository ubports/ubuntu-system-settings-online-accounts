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

#include "qml-loopback-server.h"

#include <QDebug>

using namespace OnlineAccountsPlugin;

namespace OnlineAccountsPlugin {

class QmlLoopbackServerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(QmlLoopbackServer)

public:
    QmlLoopbackServerPrivate(QmlLoopbackServer *q);
    ~QmlLoopbackServerPrivate();

    void queueUpdate();

private Q_SLOTS:
    void update();

private:
    bool m_updateQueued;
    int m_port;
    bool m_listening;
    QmlLoopbackServer *q_ptr;
};

} // namespace

QmlLoopbackServerPrivate::QmlLoopbackServerPrivate(QmlLoopbackServer *q):
    m_port(0),
    m_listening(true),
    q_ptr(q)
{
}

QmlLoopbackServerPrivate::~QmlLoopbackServerPrivate()
{
}

void QmlLoopbackServerPrivate::queueUpdate()
{
    if (!m_updateQueued) {
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
        m_updateQueued = true;
    }
}

void QmlLoopbackServerPrivate::update()
{
    Q_Q(QmlLoopbackServer);

    m_updateQueued = false;

    bool nothingToDo = m_listening == q->isListening();
    if (nothingToDo) return;

    if (m_listening) {
        if (q->listen(uint16_t(m_port))) {
            uint16_t realPort = q->port();
            if (realPort != m_port) {
                Q_EMIT q->portChanged();
            }
        } else {
            qWarning() << "Listening failed";
        }
    } else {
        q->close();
    }
}

QmlLoopbackServer::QmlLoopbackServer(QObject *parent):
    LoopbackServer(parent),
    d_ptr(new QmlLoopbackServerPrivate(this))
{
}

QmlLoopbackServer::~QmlLoopbackServer()
{
}

void QmlLoopbackServer::setPort(int port)
{
    Q_D(QmlLoopbackServer);
    if (isListening()) {
        qWarning() << "Cannot change port while already listening";
        return;
    }

    if (port == d->m_port) return;
    d->m_port = port;
    d->queueUpdate();
    Q_EMIT portChanged();
}

int QmlLoopbackServer::port() const
{
    Q_D(const QmlLoopbackServer);
    return isListening() ? LoopbackServer::port() : d->m_port;
}

void QmlLoopbackServer::setListening(bool listening)
{
    Q_D(QmlLoopbackServer);

    if (listening == d->m_listening) return;
    d->m_listening = listening;

    /* The change notification will be emitted when doing the update */
    d->queueUpdate();
}

void QmlLoopbackServer::classBegin()
{
    Q_D(QmlLoopbackServer);
    d->m_updateQueued = true;
}

void QmlLoopbackServer::componentComplete()
{
    Q_D(QmlLoopbackServer);
    d->update();
}

#include "qml-loopback-server.moc"

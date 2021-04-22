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

#ifndef OAP_QML_LOOPBACK_SERVER_H
#define OAP_QML_LOOPBACK_SERVER_H

#include "OnlineAccountsPlugin/loopback-server.h"

#include <QObject>
#include <QQmlParserStatus>
#include <QScopedPointer>

namespace OnlineAccountsPlugin {

class QmlLoopbackServerPrivate;
class QmlLoopbackServer: public LoopbackServer, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool listening READ isListening WRITE setListening
               NOTIFY callbackUrlChanged)

public:
    QmlLoopbackServer(QObject *parent = 0);
    ~QmlLoopbackServer();

    void setPort(int port);
    int port() const;

    void setListening(bool isListening);

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void portChanged();
    void callbackUrlChanged(); // to make the moc happy

private:
    Q_DECLARE_PRIVATE(QmlLoopbackServer)
    QScopedPointer<QmlLoopbackServerPrivate> d_ptr;
};

} // namespace

#endif // OAP_QML_LOOPBACK_SERVER_H

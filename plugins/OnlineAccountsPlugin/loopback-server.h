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

#ifndef OAP_LOOPBACK_SERVER_H
#define OAP_LOOPBACK_SERVER_H

#include "global.h"

#include <QByteArray>
#include <QObject>
#include <QScopedPointer>
#include <QUrl>

namespace OnlineAccountsPlugin {

class LoopbackServerPrivate;
class OAP_EXPORT LoopbackServer: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl callbackUrl READ callbackUrl NOTIFY callbackUrlChanged)
    Q_PROPERTY(QByteArray servedHtml READ servedHtml WRITE setServedHtml
               NOTIFY servedHtmlChanged)
    Q_PROPERTY(int portIncrementAttempts READ portIncrementAttempts
               WRITE setPortIncrementAttempts
               NOTIFY portIncrementAttemptsChanged)

public:
    LoopbackServer(QObject *parent = 0);
    ~LoopbackServer();

    Q_REQUIRED_RESULT bool listen(uint16_t port = 0);
    void close();

    bool isListening() const { return !callbackUrl().isEmpty(); }
    uint16_t port() const;

    void setPortIncrementAttempts(int maxAttempts);
    int portIncrementAttempts() const;

    void setServedHtml(const QByteArray &html);
    QByteArray servedHtml() const;

    QUrl callbackUrl() const;

Q_SIGNALS:
    void portIncrementAttemptsChanged();
    void servedHtmlChanged();
    void callbackUrlChanged();
    void visited(const QUrl &url);

private:
    Q_DECLARE_PRIVATE(LoopbackServer)
    QScopedPointer<LoopbackServerPrivate> d_ptr;
};

} // namespace

#endif // OAP_LOOPBACK_SERVER_H

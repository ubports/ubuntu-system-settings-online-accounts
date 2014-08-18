/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2014 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

#ifndef SIGNON_UI_REQUEST_HANDLER_H
#define SIGNON_UI_REQUEST_HANDLER_H

#include <QObject>
#include <QVariantMap>

namespace SignOnUi {

class RequestHandlerPrivate;
class Request;

class RequestHandler: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *request READ request NOTIFY requestChanged)
    Q_PROPERTY(QString matchKey READ matchKey CONSTANT)
    Q_PROPERTY(QString matchId READ matchId CONSTANT)

public:
    explicit RequestHandler(QObject *parent = 0);
    ~RequestHandler();

    void setRequest(QObject *request);
    QObject *request() const;

    static QString matchKey() { return QStringLiteral("X-RequestHandler"); }
    QString matchId() const;

    static RequestHandler *findMatching(const QVariantMap &parameters);

Q_SIGNALS:
    void requestChanged();

private:
    RequestHandlerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(RequestHandler)
};

} // namespace

#endif // SIGNON_UI_REQUEST_HANDLER_H


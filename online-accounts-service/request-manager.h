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

#ifndef OAU_REQUEST_MANAGER_H
#define OAU_REQUEST_MANAGER_H

#include <QObject>
#include <QVariantMap>

namespace OnlineAccountsUi {

class Request;
class UiProxy;

class RequestManagerPrivate;
class RequestManager: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isIdle READ isIdle NOTIFY isIdleChanged)

public:
    explicit RequestManager(QObject *parent = 0);
    ~RequestManager();

    static RequestManager *instance();

    void enqueue(Request *request);

    bool isIdle() const;

Q_SIGNALS:
    void isIdleChanged();

private:
    RequestManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(RequestManager)
};

} // namespace

#endif // OAU_REQUEST_MANAGER_H

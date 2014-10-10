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

#ifndef MOCK_SIGNON_UI_REQUEST_H
#define MOCK_SIGNON_UI_REQUEST_H

#include "signonui-request.h"

#include <OnlineAccountsPlugin/request-handler.h>
#include <QPointer>
#include <QVariantMap>

namespace SignOnUi {

class RequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Request)

public:
    RequestPrivate(Request *request);
    ~RequestPrivate();
    static RequestPrivate *mocked(Request *r) { return r->d_ptr; }

private:
    mutable Request *q_ptr;
    QVariantMap m_clientData;
    QPointer<RequestHandler> m_handler;
};

} // namespace

#endif // MOCK_SIGNON_UI_REQUEST_H

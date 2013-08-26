/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of access-control-service
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
#include "globals.h"
#include "provider-request.h"

#include <Accounts/Manager>

using namespace Acs;

namespace Acs {

class ProviderRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(ProviderRequest)

public:
    ProviderRequestPrivate(ProviderRequest *request);
    ~ProviderRequestPrivate();

    void start();

private:
    mutable ProviderRequest *q_ptr;
    QString m_providerId;
};

} // namespace

ProviderRequestPrivate::ProviderRequestPrivate(ProviderRequest *request):
    QObject(request),
    q_ptr(request)
{
    m_providerId = request->parameters().value(ACS_KEY_PROVIDER).toString();
}

ProviderRequestPrivate::~ProviderRequestPrivate()
{
}

void ProviderRequestPrivate::start()
{
    Q_Q(ProviderRequest);
    DEBUG() << "Client" << q->clientApparmorProfile();
}

ProviderRequest::ProviderRequest(const QDBusConnection &connection,
                                 const QDBusMessage &message,
                                 const QVariantMap &parameters,
                                 QObject *parent):
    Request(connection, message, parameters, parent),
    d_ptr(new ProviderRequestPrivate(this))
{
}

ProviderRequest::~ProviderRequest()
{
}

void ProviderRequest::start()
{
    Q_D(ProviderRequest);
    Request::start();
    d->start();
}

#include "provider-request.moc"

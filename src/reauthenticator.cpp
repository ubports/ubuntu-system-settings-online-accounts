/*
 * This file is part of online-accounts-ui
 *
 * Copyright (C) 2012 Canonical Ltd.
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

#include "reauthenticator.h"

#include "debug.h"

#include <SignOn/AuthSession>
#include <SignOn/Identity>

using namespace SignOnUi;
using namespace SignOn;

namespace SignOnUi {

class ReauthenticatorPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Reauthenticator)

public:
    ReauthenticatorPrivate(const QList<AuthData> &clientData,
                           const QVariantMap &extraParameters,
                           Reauthenticator *reauthenticator);
    ~ReauthenticatorPrivate();

    void start();

private:
    void checkFinished();

private Q_SLOTS:
    void onError(const SignOn::Error &error);
    void onResponse(const SignOn::SessionData &response);

private:
    mutable Reauthenticator *q_ptr;
    QList<AuthData> m_clientData;
    QVariantMap m_extraParameters;
    int m_errorCount;
    int m_responseCount;
};

} // namespace

ReauthenticatorPrivate::ReauthenticatorPrivate(
    const QList<AuthData> &clientData,
    const QVariantMap &extraParameters,
    Reauthenticator *request):
    QObject(request),
    q_ptr(request),
    m_clientData(clientData),
    m_extraParameters(extraParameters),
    m_errorCount(0),
    m_responseCount(0)
{
}

ReauthenticatorPrivate::~ReauthenticatorPrivate()
{
}

void ReauthenticatorPrivate::start()
{
    Q_FOREACH(const AuthData &authData, m_clientData) {
        Identity *identity =
            Identity::existingIdentity(authData.identity, this);
        if (identity == 0) { m_errorCount++; continue; }

        AuthSession *authSession = identity->createSession(authData.method);
        if (authSession == 0) { m_errorCount++; continue; }

        QObject::connect(authSession,
                         SIGNAL(error(const SignOn::Error &)),
                         this,
                         SLOT(onError(const SignOn::Error &)));
        QObject::connect(authSession,
                         SIGNAL(response(const SignOn::SessionData &)),
                         this,
                         SLOT(onResponse(const SignOn::SessionData &)));

        /* Prepare the session data, adding the extra parameters. */
        QVariantMap sessionData = authData.sessionData;
        QVariantMap::const_iterator i;
        for (i = m_extraParameters.constBegin();
             i != m_extraParameters.constEnd();
             i++) {
            sessionData[i.key()] = i.value();
        }

        /* Start the session right now; signon-ui is queueing them anyway. */
        authSession->process(sessionData, authData.mechanism);
    }

    checkFinished();
}

void ReauthenticatorPrivate::checkFinished()
{
    Q_Q(Reauthenticator);

    if (m_errorCount + m_responseCount < m_clientData.count()) return;
    Q_EMIT q->finished(m_errorCount == 0);
}

void ReauthenticatorPrivate::onError(const SignOn::Error &error)
{
    DEBUG() << "Got error:" << error.message();

    m_errorCount++;
    checkFinished();
}

void ReauthenticatorPrivate::onResponse(
                                    const SignOn::SessionData &response)
{
    DEBUG() << "Got response:" << response.toMap();

    m_responseCount++;
    checkFinished();
}

Reauthenticator::Reauthenticator(const QList<AuthData> &clientData,
                                 const QVariantMap &extraParameters,
                                 QObject *parent):
    QObject(parent),
    d_ptr(new ReauthenticatorPrivate(clientData, extraParameters, this))
{
}

Reauthenticator::~Reauthenticator()
{
}

void Reauthenticator::start()
{
    Q_D(Reauthenticator);
    d->start();
}

#include "reauthenticator.moc"

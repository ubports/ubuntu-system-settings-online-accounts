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
#include "mir-helper.h"

#include <mir_toolkit/mir_client_library.h>
#include <mir_toolkit/mir_prompt_session.h>

#include <QList>
#include <QStandardPaths>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

static MirHelper *m_instance = 0;

class PromptSessionPrivate
{
public:
    inline PromptSessionPrivate(MirPromptSession *session);
    inline ~PromptSessionPrivate();

    void emitFinished() { Q_EMIT q_ptr->finished(); }

    MirPromptSession *m_mirSession;
    QList<int> m_fds;
    mutable PromptSession *q_ptr;
};

class MirHelperPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(MirHelper)

public:
    inline MirHelperPrivate(MirHelper *helper);
    inline ~MirHelperPrivate();

    PromptSession *createPromptSession(pid_t initiatorPid);
    void onSessionStopped(MirPromptSession *mirSession);

private:
    friend class PromptSession;
    MirConnection *m_connection;
    QList<PromptSession*> m_sessions;
    mutable MirHelper *q_ptr;
};

} // namespace

PromptSessionPrivate::PromptSessionPrivate(MirPromptSession *session):
    m_mirSession(session)
{
}

PromptSessionPrivate::~PromptSessionPrivate()
{
    DEBUG();
    mir_prompt_session_release_sync(m_mirSession);
    m_mirSession = 0;
}

PromptSession::PromptSession(PromptSessionPrivate *priv):
    d_ptr(priv)
{
    priv->q_ptr = this;
    MirHelperPrivate *helperPrivate = MirHelper::instance()->d_ptr;
    helperPrivate->m_sessions.append(this);
}

PromptSession::~PromptSession()
{
    MirHelperPrivate *helperPrivate = MirHelper::instance()->d_ptr;
    helperPrivate->m_sessions.removeOne(this);
    delete d_ptr;
}

static void client_fd_callback(MirPromptSession *, size_t count,
                               int const *fds, void *context)
{
    PromptSessionPrivate *priv = (PromptSessionPrivate *)context;
    for (size_t i = 0; i < count; i++) {
        priv->m_fds.append(fds[i]);
    }
}

QString PromptSession::requestSocket()
{
    Q_D(PromptSession);

    d->m_fds.clear();
    mir_wait_for(mir_prompt_session_new_fds_for_prompt_providers(
        d->m_mirSession, 1, client_fd_callback, d));
    if (!d->m_fds.isEmpty()) {
        return QString("fd://%1").arg(d->m_fds[0]);
    } else {
        return QString();
    }
}

MirHelperPrivate::MirHelperPrivate(MirHelper *helper):
    QObject(helper),
    q_ptr(helper)
{
    QString mirSocket =
        QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) +
        "/mir_socket_trusted";
    m_connection = mir_connect_sync(mirSocket.toUtf8().constData(),
                                    "online-accounts-service");
    if (Q_UNLIKELY(!mir_connection_is_valid(m_connection))) {
        qWarning() << "Invalid Mir connection:" <<
            mir_connection_get_error_message(m_connection);
        return;
    }
}

MirHelperPrivate::~MirHelperPrivate()
{
    if (m_connection) {
        mir_connection_release(m_connection);
        m_connection = 0;
    }
}

static void session_event_callback(MirPromptSession *mirSession,
                                   MirPromptSessionState state,
                                   void *self)
{
    MirHelperPrivate *helper = reinterpret_cast<MirHelperPrivate*>(self);
    DEBUG() << "Prompt Session state updated to" << state;
    if (state == mir_prompt_session_state_stopped) {
        helper->onSessionStopped(mirSession);
    }
}

void MirHelperPrivate::onSessionStopped(MirPromptSession *mirSession)
{
    Q_FOREACH(PromptSession *session, m_sessions) {
        if (mirSession == session->d_ptr->m_mirSession) {
            session->d_ptr->emitFinished();
        }
    }
}

PromptSession *MirHelperPrivate::createPromptSession(pid_t initiatorPid)
{
    if (Q_UNLIKELY(!m_connection)) return 0;

    MirPromptSession *mirSession =
        mir_connection_create_prompt_session_sync(m_connection,
                                                  initiatorPid,
                                                  session_event_callback,
                                                  this);
    if (!mirSession) return 0;

    if (Q_UNLIKELY(!mir_prompt_session_is_valid(mirSession))) {
        qWarning() << "Invalid prompt session:" <<
            mir_prompt_session_error_message(mirSession);
        return 0;
    }

    return new PromptSession(new PromptSessionPrivate(mirSession));
}

MirHelper::MirHelper(QObject *parent):
    QObject(parent),
    d_ptr(new MirHelperPrivate(this))
{
}

MirHelper::~MirHelper()
{
    m_instance = 0;
}

MirHelper *MirHelper::instance()
{
    if (!m_instance) {
        m_instance = new MirHelper;
    }
    return m_instance;
}

PromptSession *MirHelper::createPromptSession(pid_t initiatorPid)
{
    Q_D(MirHelper);
    return d->createPromptSession(initiatorPid);
}

#include "mir-helper.moc"

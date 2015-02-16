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

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

static MirHelper *m_instance = 0;

} // namespace

PromptSession::PromptSession(PromptSessionPrivate *priv):
    d_ptr(priv)
{
}

PromptSession::~PromptSession()
{
}

QString PromptSession::requestSocket()
{
#ifdef BUILDING_TESTS
    return QString::fromUtf8(qgetenv("TEST_MIR_HELPER_SOCKET"));
#else
    return QString();
#endif
}

MirHelper::MirHelper(QObject *parent):
    QObject(parent),
    d_ptr(0)
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

PromptSessionP MirHelper::createPromptSession(pid_t initiatorPid)
{
    Q_UNUSED(initiatorPid);
#ifdef BUILDING_TESTS
    return qgetenv("TEST_MIR_HELPER_FAIL_CREATE").isEmpty() ?
        PromptSessionP(new PromptSession(0)) : PromptSessionP();
#else
    return PromptSessionP();
#endif
}

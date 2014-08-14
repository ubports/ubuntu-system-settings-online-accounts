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

#ifndef OAU_MIR_HELPER_H
#define OAU_MIR_HELPER_H

#include <QObject>

namespace OnlineAccountsUi {

class PromptSessionPrivate;
class MirHelperPrivate;

class PromptSession
{
public:
    ~PromptSession();

    QString requestSocket();

private:
    explicit PromptSession(PromptSessionPrivate *priv);

private:
    friend class MirHelperPrivate;
    PromptSessionPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PromptSession)
};

class MirHelper: public QObject
{
    Q_OBJECT

public:
    static MirHelper *instance();

    PromptSession *createPromptSession(pid_t initiatorPid);

private:
    explicit MirHelper(QObject *parent = 0);
    ~MirHelper();

private:
    friend class PromptSession;
    MirHelperPrivate *d_ptr;
    Q_DECLARE_PRIVATE(MirHelper)
};

} // namespace

#endif // OAU_MIR_HELPER_H

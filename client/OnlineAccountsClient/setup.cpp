/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of OnlineAccountsClient.
 *
 * OnlineAccountsClient is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * OnlineAccountsClient is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OnlineAccountsClient.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "setup.h"

#include <QProcess>

using namespace OnlineAccountsClient;

namespace OnlineAccountsClient {

class SetupPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Setup)

public:
    inline SetupPrivate(Setup *setup);
    ~SetupPrivate() {};

    void exec();

private:
    QProcess m_process;
    QString m_serviceTypeId;
    QString m_providerId;
    mutable Setup *q_ptr;
};

}; // namespace

SetupPrivate::SetupPrivate(Setup *setup):
    QObject(setup),
    q_ptr(setup)
{
    QObject::connect(&m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
                     setup, SIGNAL(finished()));
    QObject::connect(&m_process, SIGNAL(error(QProcess::ProcessError)),
                     setup, SIGNAL(finished()));
}

void SetupPrivate::exec()
{
    QStringList args;

    if (!m_serviceTypeId.isEmpty()) {
        args.append("--option");
        args.append(QString::fromLatin1("serviceType=%1").arg(m_serviceTypeId));
    }

    if (!m_providerId.isEmpty()) {
        args.append("--option");
        args.append(QString::fromLatin1("provider=%1").arg(m_providerId));
    }

    args.append("online-accounts");
    m_process.start("system-settings", args);
}

/*!
 * \qmltype Setup
 * \inqmlmodule Ubuntu.OnlineAccounts.Client 0.1
 * \ingroup Ubuntu
 *
 * \brief Invoke the Online Accounts panel
 *
 * This object can be used by applications to request the creation of an
 * account. By calling the \l exec() method, the Online Accounts panel will
 * appear and guide the user through the creation of an account. Once done, the
 * \l finished() signal will be emitted. The type of account to be created can
 * be configured by this object's properties.
 *
 * Example:
 *
 * \qml
 * import QtQuick 2.0
 * import Ubuntu.Components 0.1
 * import Ubuntu.OnlineAccounts.Client 0.1
 *
 * Rectangle {
 *     width: 400
 *     height: 300
 *
 *     Button {
 *         anchors.centerIn: parent
 *         text: "Create Facebook account"
 *         onClicked: setup.exec()
 *     }
 *
 *     Setup {
 *         id: setup
 *         providerId: "facebook"
 *     }
 * }
 * \endqml
 */
Setup::Setup(QObject *parent):
    QObject(parent),
    d_ptr(new SetupPrivate(this))
{
}

Setup::~Setup()
{
    delete d_ptr;
}

/*!
 * \qmlproperty string Setup::serviceTypeId
 * If set to a valid service type, the user will be asked to create an Online
 * Account which supports this service type.
 */
void Setup::setServiceTypeId(const QString &serviceTypeId)
{
    Q_D(Setup);
    if (serviceTypeId == d->m_serviceTypeId) return;
    d->m_serviceTypeId = serviceTypeId;
    Q_EMIT serviceTypeIdChanged();
}

QString Setup::serviceTypeId() const
{
    Q_D(const Setup);
    return d->m_serviceTypeId;
}

/*!
 * \qmlproperty string Setup::providerId
 * If set to a valid provider, the user will be asked to create an Online
 * Account provided by this entity.
 */
void Setup::setProviderId(const QString &providerId)
{
    Q_D(Setup);
    if (providerId == d->m_providerId) return;
    d->m_providerId = providerId;
    Q_EMIT providerIdChanged();
}

QString Setup::providerId() const
{
    Q_D(const Setup);
    return d->m_providerId;
}

/*!
 * \qmlmethod void Setup::exec()
 * Launches the Online Accounts panel.
 */
void Setup::exec()
{
    Q_D(Setup);
    d->exec();
}

/*!
 * \qmlsignal Setup::finished()
 * Emitted when the Online Accounts panel has been closed.
 */

#include "setup.moc"

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

#include "ui-server-mock.h"

#include <QDebug>

using namespace OnlineAccountsUi;

static UiServer *m_instance = 0;

UiServerPrivate::UiServerPrivate(const QString &address,
                                 UiServer *server):
    QObject(server),
    q_ptr(server),
    m_address(address)
{
}

UiServerPrivate::~UiServerPrivate()
{
}

UiServer::UiServer(const QString &address,
                   QObject *parent):
    QObject(parent),
    d_ptr(new UiServerPrivate(address, this))
{
    m_instance = this;
}

UiServer::~UiServer()
{
    m_instance = 0;
}

UiServer *UiServer::instance()
{
    return m_instance;
}

bool UiServer::init()
{
    return true;
}

void UiServer::registerHandler(SignOnUi::RequestHandler *handler)
{
    Q_D(UiServer);
    Q_EMIT d->registerHandlerCalled(handler);
}

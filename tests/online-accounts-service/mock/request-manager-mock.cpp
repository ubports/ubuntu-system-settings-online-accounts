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

#include "request-manager-mock.h"

#include <QDebug>

using namespace OnlineAccountsUi;

static RequestManager *m_instance = 0;

RequestManagerPrivate::RequestManagerPrivate(RequestManager *q):
    QObject(q),
    q_ptr(q)
{
}

RequestManagerPrivate::~RequestManagerPrivate()
{
}

RequestManager::RequestManager(QObject *parent):
    QObject(parent),
    d_ptr(new RequestManagerPrivate(this))
{
    m_instance = this;
}

RequestManager::~RequestManager()
{
    m_instance = 0;
}

RequestManager *RequestManager::instance()
{
    return m_instance;
}

bool RequestManager::isIdle() const
{
    return true;
}

void RequestManager::enqueue(Request *request)
{
    Q_D(RequestManager);
    Q_EMIT d->enqueueCalled(request);
}

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

#include "plugin.h"
#include <OnlineAccountsClient/Setup>
#include <QDebug>
#include <QQmlComponent>

using namespace OnlineAccountsClient;

void Plugin::registerTypes(const char *uri)
{
    qDebug() << Q_FUNC_INFO << uri;
    qmlRegisterType<Setup>(uri, 0, 1, "Setup");
    qmlRegisterType<Setup>(uri, 0, 2, "Setup");
}

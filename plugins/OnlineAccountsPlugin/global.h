/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of OnlineAccountsPlugin.
 *
 * OnlineAccountsPlugin is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * OnlineAccountsPlugin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OnlineAccountsPlugin.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef ONLINE_ACCOUNTS_PLUGIN_GLOBAL_H
#define ONLINE_ACCOUNTS_PLUGIN_GLOBAL_H

#include <QtCore/QtGlobal>

#if defined(BUILDING_ONLINE_ACCOUNTS_PLUGIN)
#  define OAP_EXPORT Q_DECL_EXPORT
#else
#  define OAP_EXPORT Q_DECL_IMPORT
#endif

#endif // ONLINE_ACCOUNTS_PLUGIN_GLOBAL_H

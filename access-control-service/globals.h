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

#ifndef ACS_GLOBALS_H
#define ACS_GLOBALS_H

#define ACCESS_CONTROL_SERVICE_NAME \
    QStringLiteral("com.canonical.OnlineAccounts.AccessControl")
#define ACCESS_CONTROL_OBJECT_PATH \
    QStringLiteral("/")

#define ACS_KEY_PROVIDER            QStringLiteral("provider")
#define ACS_KEY_SERVICE_TYPE        QStringLiteral("serviceType")
#define ACS_KEY_WINDOW_ID           QStringLiteral("windowId")

// D-Bus error names
#define ACS_ERROR_PREFIX "com.canonical.OnlineAccounts.AccessControl."
#define ACS_ERROR_USER_CANCELED \
    QStringLiteral(ACS_ERROR_PREFIX "UserCanceled")
#define ACS_ERROR_INVALID_PARAMETERS \
    QStringLiteral(ACS_ERROR_PREFIX "InvalidParameters")

#endif // ACS_GLOBALS_H

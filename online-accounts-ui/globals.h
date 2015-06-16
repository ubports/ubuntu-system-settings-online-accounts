/*
 * Copyright (C) 2013 Canonical Ltd.
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

#ifndef OAU_GLOBALS_H
#define OAU_GLOBALS_H

#define OAU_SERVICE_NAME    QStringLiteral("com.ubuntu.OnlineAccountsUi")
#define OAU_OBJECT_PATH     QStringLiteral("/")
#define OAU_INTERFACE       OAU_SERVICE_NAME

#define OAU_KEY_APPLICATION         QStringLiteral("application")
#define OAU_KEY_PROVIDER            QStringLiteral("provider")
#define OAU_KEY_SERVICE_ID          QStringLiteral("serviceId")
#define OAU_KEY_SERVICE_TYPE        QStringLiteral("serviceType")
#define OAU_KEY_WINDOW_ID           QStringLiteral("windowId")
#define OAU_KEY_PID                 QStringLiteral("pid")
#define OAU_KEY_ACCOUNT_ID          QStringLiteral("accountId")

// D-Bus error names
#define OAU_ERROR_PREFIX "com.ubuntu.OnlineAccountsUi."
#define OAU_ERROR_USER_CANCELED \
    QStringLiteral(OAU_ERROR_PREFIX "UserCanceled")
#define OAU_ERROR_INVALID_PARAMETERS \
    QStringLiteral(OAU_ERROR_PREFIX "InvalidParameters")
#define OAU_ERROR_INVALID_APPLICATION \
    QStringLiteral(OAU_ERROR_PREFIX "InvalidApplication")
#define OAU_ERROR_INVALID_SERVICE \
    QStringLiteral(OAU_ERROR_PREFIX "InvalidService")
#define OAU_ERROR_PROMPT_SESSION \
    QStringLiteral(OAU_ERROR_PREFIX "NoPromptSession")

/* SignOnUi service */
#define SIGNONUI_SERVICE_NAME   QStringLiteral("com.nokia.singlesignonui")
#define SIGNONUI_OBJECT_PATH    QStringLiteral("/SignonUi")
#define SIGNONUI_INTERFACE      SIGNONUI_SERVICE_NAME

#define SIGNON_UI_ERROR_PREFIX "com.ubuntu.SignonUi."

#define SIGNON_UI_ERROR_INTERNAL \
    QStringLiteral(SIGNON_UI_ERROR_PREFIX "InternalError")

#endif // OAU_GLOBALS_H

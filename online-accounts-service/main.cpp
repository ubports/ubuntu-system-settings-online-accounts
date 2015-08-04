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

#include "debug.h"
#include "globals.h"
#include "inactivity-timer.h"
#include "indicator-service.h"
#include "libaccounts-service.h"
#include "request-manager.h"
#include "service.h"
#include "signonui-service.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QProcessEnvironment>

using namespace OnlineAccountsUi;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    /* read environment variables */
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains(QLatin1String("OAU_LOGGING_LEVEL"))) {
        bool isOk;
        int value = environment.value(
            QLatin1String("OAU_LOGGING_LEVEL")).toInt(&isOk);
        if (isOk)
            setLoggingLevel(value);
    }

    /* default daemonTimeout to 5 seconds */
    int daemonTimeout = 5;

    /* override daemonTimeout if OAU_DAEMON_TIMEOUT is set */
    if (environment.contains(QLatin1String("OAU_DAEMON_TIMEOUT"))) {
        bool isOk;
        int value = environment.value(
            QLatin1String("OAU_DAEMON_TIMEOUT")).toInt(&isOk);
        if (isOk)
            daemonTimeout = value;
    }

    RequestManager *requestManager = new RequestManager();

    qDBusRegisterMetaType<SignOnUi::RawCookies>();

    Service *service = new Service();
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.registerObject(OAU_OBJECT_PATH, service);
    connection.registerService(OAU_SERVICE_NAME);

    SignOnUi::Service *signonuiService = new SignOnUi::Service();
    connection.registerObject(SIGNONUI_OBJECT_PATH, signonuiService,
                              QDBusConnection::ExportAllContents);
    connection.registerService(SIGNONUI_SERVICE_NAME);

    SignOnUi::IndicatorService *indicatorService =
        new SignOnUi::IndicatorService();
    connection.registerObject(WEBCREDENTIALS_OBJECT_PATH,
                              indicatorService->serviceObject());
    connection.registerService(WEBCREDENTIALS_BUS_NAME);

    LibaccountsService *libaccountsService = new LibaccountsService();
    connection.registerObject(LIBACCOUNTS_OBJECT_PATH, libaccountsService,
                              QDBusConnection::ExportAllContents);
    connection.registerService(LIBACCOUNTS_BUS_NAME);

    InactivityTimer *inactivityTimer = 0;
    if (daemonTimeout > 0) {
        inactivityTimer = new InactivityTimer(daemonTimeout * 1000);
        inactivityTimer->watchObject(requestManager);
        inactivityTimer->watchObject(indicatorService);
        QObject::connect(inactivityTimer, SIGNAL(timeout()),
                         &app, SLOT(quit()));
    }

    int ret = app.exec();

    connection.unregisterService(LIBACCOUNTS_BUS_NAME);
    connection.unregisterObject(LIBACCOUNTS_OBJECT_PATH);
    delete libaccountsService;

    connection.unregisterService(WEBCREDENTIALS_BUS_NAME);
    connection.unregisterObject(WEBCREDENTIALS_OBJECT_PATH);
    delete indicatorService;

    connection.unregisterService(SIGNONUI_SERVICE_NAME);
    connection.unregisterObject(SIGNONUI_OBJECT_PATH);
    delete signonuiService;

    connection.unregisterService(OAU_SERVICE_NAME);
    connection.unregisterObject(OAU_OBJECT_PATH);
    delete service;

    delete requestManager;

    delete inactivityTimer;

    return ret;
}


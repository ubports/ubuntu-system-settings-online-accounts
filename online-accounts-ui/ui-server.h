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

#ifndef OAU_UI_SERVER_H
#define OAU_UI_SERVER_H

#include <QObject>
#include <QVariantMap>

namespace SignOnUi {
class RequestHandler;
}

namespace OnlineAccountsUi {

class UiServerPrivate;
class UiServer: public QObject
{
    Q_OBJECT

public:
    explicit UiServer(const QString &address, QObject *parent = 0);
    ~UiServer();

    static UiServer *instance();

    bool init();
    void registerHandler(SignOnUi::RequestHandler *handler);

Q_SIGNALS:
    void finished();

private:
    UiServerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(UiServer)
};

} // namespace

#endif // OAU_UI_SERVER_H

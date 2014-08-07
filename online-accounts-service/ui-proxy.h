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

#ifndef OAU_UI_PROXY_H
#define OAU_UI_PROXY_H

#include <QObject>
#include <QVariantMap>

namespace OnlineAccountsUi {

class Request;

class UiProxyPrivate;
class UiProxy: public QObject
{
    Q_OBJECT

public:
    explicit UiProxy(QObject *parent = 0);
    ~UiProxy();

    bool init();
    void handleRequest(Request *request);
    bool hasHandlerFor(const QVariantMap &parameters);

Q_SIGNALS:
    void finished();

private:
    UiProxyPrivate *d_ptr;
    Q_DECLARE_PRIVATE(UiProxy)
};

} // namespace

#endif // OAU_UI_PROXY_H

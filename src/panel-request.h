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

#ifndef OAU_PANEL_REQUEST_H
#define OAU_PANEL_REQUEST_H

#include "request.h"

namespace OnlineAccountsUi {

class PanelRequestPrivate;
class PanelRequest: public Request
{
    Q_OBJECT

public:
    explicit PanelRequest(const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent = 0);
    ~PanelRequest();

    void start() Q_DECL_OVERRIDE;

private:
    PanelRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(PanelRequest)
};

} // namespace

#endif // OAU_PANEL_REQUEST_H

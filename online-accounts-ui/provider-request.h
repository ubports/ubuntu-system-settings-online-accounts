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

#ifndef OAU_PROVIDER_REQUEST_H
#define OAU_PROVIDER_REQUEST_H

#include "request.h"

namespace OnlineAccountsUi {

class ProviderRequestPrivate;
class ProviderRequest: public Request
{
    Q_OBJECT

public:
    explicit ProviderRequest(const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent = 0);
    ~ProviderRequest();

    void start() Q_DECL_OVERRIDE;

private:
    ProviderRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(ProviderRequest)
};

} // namespace

#endif // OAU_PROVIDER_REQUEST_H

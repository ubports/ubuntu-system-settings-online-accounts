/*
 * This file is part of online-accounts-ui
 *
 * Copyright (C) 2014 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

#ifndef SIGNON_UI_BROWSER_REQUEST_H
#define SIGNON_UI_BROWSER_REQUEST_H

#include "signonui-request.h"

#include <QObject>

namespace SignOnUi {

class BrowserRequestPrivate;

class BrowserRequest: public Request
{
    Q_OBJECT

public:
    explicit BrowserRequest(int id,
                            const QString &clientProfile,
                            const QVariantMap &parameters,
                            QObject *parent = 0);
    ~BrowserRequest();

    // reimplemented virtual methods
    void start();

private:
    BrowserRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(BrowserRequest)
};

} // namespace

#endif // SIGNON_UI_BROWSER_REQUEST_H


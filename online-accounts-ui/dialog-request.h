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

#ifndef SIGNON_UI_DIALOG_REQUEST_H
#define SIGNON_UI_DIALOG_REQUEST_H

#include "signonui-request.h"

#include <QObject>

namespace SignOnUi {

class DialogRequestPrivate;

class DialogRequest: public Request
{
    Q_OBJECT

public:
    explicit DialogRequest(int id,
                           const QString &clientProfile,
                           const QVariantMap &parameters,
                           QObject *parent = 0);
    ~DialogRequest();

    // reimplemented virtual methods
    void start();

private:
    DialogRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(DialogRequest)
};

} // namespace

#endif // SIGNON_UI_DIALOG_REQUEST_H


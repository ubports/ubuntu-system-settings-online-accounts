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

#ifndef SIGNON_UI_DIALOG_H
#define SIGNON_UI_DIALOG_H

#include <QObject>
#include <QQuickView>

namespace SignOnUi {

class Dialog: public QQuickView
{
    Q_OBJECT

public:
    enum DialogCode {
        Rejected = 0,
        Accepted,
    };
    enum ShowMode {
        TopLevel = 0,
        Transient,
        Embedded,
    };
    explicit Dialog(QWindow *parent = 0);
    ~Dialog();

    void show(WId parent, ShowMode mode);

public Q_SLOTS:
    void accept();
    void reject();
    void done(int result);

Q_SIGNALS:
    void finished(int result);

protected:
    bool event(QEvent *e);
};

} // namespace

#endif // SIGNON_UI_DIALOG_H

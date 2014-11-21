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

#ifndef OAU_REQUEST_H
#define OAU_REQUEST_H

#include <QObject>
#include <QVariantMap>
#include <QWindow>

namespace OnlineAccountsUi {

class RequestPrivate;
class Request: public QObject
{
    Q_OBJECT

public:
    static Request *newRequest(const QString &interface,
                               int id,
                               const QString &clientProfile,
                               const QVariantMap &parameters,
                               QObject *parent = 0);
    ~Request();

    static Request *find(const QVariantMap &match);

    QString interface() const;
    int id() const;
    WId windowId() const;
    bool isInProgress() const;
    const QVariantMap &parameters() const;
    QString clientApparmorProfile() const;
    QWindow *window() const;

    QVariantMap result() const;
    QString errorName() const;
    QString errorMessage() const;
    int delay() const;

public Q_SLOTS:
    virtual void start();
    void cancel();

Q_SIGNALS:
    void completed();

protected:
    explicit Request(const QString &interface,
                     int id,
                     const QString &clientProfile,
                     const QVariantMap &parameters,
                     QObject *parent = 0);
    virtual void setWindow(QWindow *window);
    void setDelay(int delay);

protected Q_SLOTS:
    void fail(const QString &name, const QString &message);
    virtual void setCanceled();
    void setResult(const QVariantMap &result);

private:
    RequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Request)
};

} // namespace

#endif // OAU_REQUEST_H

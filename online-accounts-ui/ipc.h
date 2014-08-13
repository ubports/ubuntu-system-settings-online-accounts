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

#ifndef OAU_IPC_H
#define OAU_IPC_H

#include <QObject>

class QByteArray;
class QIODevice;

#define OAU_OPERATION_CODE "code"
#define OAU_OPERATION_CODE_PROCESS "process"
#define OAU_OPERATION_CODE_REGISTER_HANDLER "newHandler"
#define OAU_OPERATION_CODE_REQUEST_FINISHED "finished"
#define OAU_OPERATION_CODE_REQUEST_FAILED "failed"
#define OAU_OPERATION_ID "id"
#define OAU_OPERATION_DATA "data"
#define OAU_OPERATION_INTERFACE "interface"
#define OAU_OPERATION_CLIENT_PROFILE "profile"
#define OAU_OPERATION_ERROR_NAME "errname"
#define OAU_OPERATION_ERROR_MESSAGE "errmsg"
#define OAU_OPERATION_HANDLER_ID "handlerId"
#define OAU_REQUEST_MATCH_KEY "X-RequestHandler"

namespace OnlineAccountsUi {

class IpcPrivate;
class Ipc: public QObject
{
    Q_OBJECT

public:
    Ipc(QObject *parent = 0);
    ~Ipc();

    void setChannels(QIODevice *readChannel, QIODevice *writeChannel);
    void write(const QByteArray &data);

Q_SIGNALS:
    void dataReady(QByteArray &data);

private:
    IpcPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Ipc)
};

}; // namespace

#endif // OAU_IPC_H

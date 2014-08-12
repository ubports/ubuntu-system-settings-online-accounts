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

#include "ipc.h"

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QSocketNotifier>

using namespace OnlineAccountsUi;

static const QByteArray welcomeMessage = "OAUinitIPC";

namespace OnlineAccountsUi {

class IpcPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Ipc)

public:
    inline IpcPrivate(Ipc *ipc);
    ~IpcPrivate() {};

    void setChannels(QIODevice *readChannel, QIODevice *writeChannel);

private Q_SLOTS:
    void onReadyRead();

private:
    bool waitWelcomeMessage();

private:
    QIODevice *m_readChannel;
    QIODevice *m_writeChannel;
    int m_expectedLength;
    bool m_gotWelcomeMessage;
    QByteArray m_readBuffer;
    mutable Ipc *q_ptr;
};

}; // namespace

IpcPrivate::IpcPrivate(Ipc *ipc):
    QObject(ipc),
    m_readChannel(0),
    m_writeChannel(0),
    m_expectedLength(0),
    m_gotWelcomeMessage(false),
    q_ptr(ipc)
{
}

void IpcPrivate::setChannels(QIODevice *readChannel, QIODevice *writeChannel)
{
    m_readChannel = readChannel;
    m_writeChannel = writeChannel;
    QObject::connect(m_readChannel, SIGNAL(readyRead()),
                     this, SLOT(onReadyRead()));
    /* QFile need special handling */
    QFile *file = qobject_cast<QFile*>(m_readChannel);
    if (file != 0) {
        QSocketNotifier *notifier = new QSocketNotifier(file->handle(),
                                                        QSocketNotifier::Read,
                                                        this);
        QObject::connect(notifier, SIGNAL(activated(int)),
                         this, SLOT(onReadyRead()));
    } else {
        /* If the read channel is not a QFile, it means it's not the standard
         * input, therefore we won't have the need to wait for the welcome
         * message. */
        m_gotWelcomeMessage = true;
    }
    onReadyRead();

    file = qobject_cast<QFile*>(m_writeChannel);
    if (file != 0) {
        m_writeChannel->write(welcomeMessage);
    }
}

void IpcPrivate::onReadyRead()
{
    Q_Q(Ipc);

    while (true) {
        if (m_expectedLength == 0) {
            /* We are beginning a new read */

            /* skip all noise */
            if (!waitWelcomeMessage()) break;

            int length;
            int bytesRead = m_readChannel->read((char *)&length,
                                                sizeof(length));
            if (bytesRead < int(sizeof(length))) break;
            m_expectedLength = length;
            m_readBuffer.clear();
        }

        int neededBytes = m_expectedLength - m_readBuffer.length();
        QByteArray buffer = m_readChannel->read(neededBytes);
        m_readBuffer += buffer;
        if (buffer.length() < neededBytes) break;
        if (m_readBuffer.length() == m_expectedLength) {
            Q_EMIT q->dataReady(m_readBuffer);
            m_expectedLength = 0;
        }
    }
}

bool IpcPrivate::waitWelcomeMessage()
{
    if (m_gotWelcomeMessage) return true;

    /* All Qt applications on the Nexus 4 write some dust to stdout when
     * starting. So, skip all input until a well-defined welcome message is
     * found */

    QByteArray buffer;
    int startCheckIndex = 0;
    do {
        buffer = m_readChannel->peek(40);
        int found = buffer.indexOf(welcomeMessage, startCheckIndex);
        int skip = (found >= 0) ? found : buffer.length() - welcomeMessage.length();
        if (found >= 0) {
            buffer = m_readChannel->read(skip + welcomeMessage.length());
            m_gotWelcomeMessage = true;
            return true;
        }
        if (skip > 0) {
            buffer = m_readChannel->read(skip);
        } else {
            buffer.clear();
        }
    } while (!buffer.isEmpty());

    return false;
}

Ipc::Ipc(QObject *parent):
    QObject(parent),
    d_ptr(new IpcPrivate(this))
{
}

Ipc::~Ipc()
{
}

void Ipc::setChannels(QIODevice *readChannel, QIODevice *writeChannel)
{
    Q_D(Ipc);
    d->setChannels(readChannel, writeChannel);
}

void Ipc::write(const QByteArray &data)
{
    Q_D(Ipc);
    int length = data.count();
    d->m_writeChannel->write((char *)&length, sizeof(length));
    d->m_writeChannel->write(data);
    d->m_writeChannel->waitForBytesWritten(-1);
}

#include "ipc.moc"

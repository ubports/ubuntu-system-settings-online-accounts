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

#ifndef WINDOW_WATCHER_H
#define WINDOW_WATCHER_H

#include <QObject>
#include <QWindow>

class WindowWatcher: public QObject
{
    Q_OBJECT

public:
    static WindowWatcher *instance() {
        if (!m_instance) {
            m_instance = new WindowWatcher;
        }
        return m_instance;
    }

    void emitWindowShown(QWindow *w) {
        Q_EMIT windowShown(w);
    }

Q_SIGNALS:
    void windowShown(QObject *);

protected:
    WindowWatcher(): QObject() {}

private:
    static WindowWatcher *m_instance;
};

#endif // WINDOW_WATCHER_H

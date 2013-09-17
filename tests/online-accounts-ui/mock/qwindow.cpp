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

#include "window-watcher.h"

#include <QDebug>
#include <QWindow>

#define TRANSIENT_PARENT    "transientParent"
#define WIN_ID              "winId"

WindowWatcher *WindowWatcher::m_instance = 0;

QWindow::QWindow(QWindow *parent):
    QObject(parent),
    QSurface(QSurface::Window)
{
    qDebug() << Q_FUNC_INFO;
}

QWindow::~QWindow()
{
}

void QWindow::show()
{
    WindowWatcher::instance()->emitWindowShown(this);
}

void QWindow::setTransientParent(QWindow *parent)
{
    setProperty(TRANSIENT_PARENT, QVariant::fromValue<QObject*>(parent));
}

QWindow *QWindow::fromWinId(WId winId)
{
    QWindow *window = new QWindow;
    window->setProperty(WIN_ID, winId);
    return window;
}

/*
 * Copyright (C) 2013 Canonical Ltd.
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

#include "plugin.h"

#include <OnlineAccountsClient/Setup>
#include <QDebug>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QStringList>
#include <SystemSettings/ItemBase>

using namespace SystemSettings;

class Item: public ItemBase
{
    Q_OBJECT

public:
    Item(const QVariantMap &staticData, QObject *parent = 0);
    ~Item();

    QQmlComponent *pageComponent(QQmlEngine *engine,
                                 QObject *parent = 0) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onFinished();

private:
    OnlineAccountsClient::Setup m_setup;
    bool m_isOpen;
};

Item::Item(const QVariantMap &staticData, QObject *parent):
    ItemBase(staticData, parent),
    m_isOpen(false)
{
    QObject::connect(&m_setup, SIGNAL(finished()),
                     this, SLOT(onFinished()));
}

Item::~Item()
{
}

QQmlComponent *Item::pageComponent(QQmlEngine *engine,
                                   QObject *parent)
{
    Q_UNUSED(engine);
    Q_UNUSED(parent);

    /* FIXME HACK: remove when window reparenting is implemented */
    if (QGuiApplication::platformName().startsWith("ubuntu")) {
        QDesktopServices::openUrl(QUrl("application:///online-accounts-ui.desktop"));
    }

    if (!m_isOpen) {
        qDebug() << "Opening Online Accounts";
        m_isOpen = true;
        m_setup.exec();
    }
    return 0;
}

void Item::onFinished()
{
    m_isOpen = false;
}

Plugin::Plugin():
    QObject()
{
}

ItemBase *Plugin::createItem(const QVariantMap &staticData,
                             QObject *parent)
{
    return new Item(staticData, parent);
}

#include "plugin.moc"

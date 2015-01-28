/*
 * Copyright (C) 2015 Canonical Ltd.
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

#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Provider>
#include <Accounts/Service>
#include <QDebug>
#include <QStringList>
#include <SystemSettings/ItemBase>
#include <libintl.h>

using namespace SystemSettings;

class Item: public ItemBase
{
    Q_OBJECT

public:
    Item(const QVariantMap &staticData, QObject *parent = 0);
    ~Item();

private:
    void computeKeywords();
};

Item::Item(const QVariantMap &staticData, QObject *parent):
    ItemBase(staticData, parent)
{
    computeKeywords();
}

Item::~Item()
{
}

static QStringList translations(const QString &text, const QString &domain)
{
    /* Return a list of keywords based on a translatable name:
     * - the untranslated text (lowercase, split into words)
     * - the translated text (lowercase, split into words)
     */
    QStringList keys;
    keys = text.toLower().split(" ");
    if (!domain.isEmpty()) {
        QByteArray baText = text.toUtf8();
        QByteArray baDomain = domain.toUtf8();
        QString translated = QString::fromUtf8(dgettext(baDomain.constData(),
                                                        baText.constData()));
        if (translated != text) {
            keys.append(translated.toLower().split(" "));
        }
    }
    return keys;
}

void Item::computeKeywords()
{
    Accounts::Manager *manager = new Accounts::Manager;

    QStringList keywords;

    /* List available providers, and add their names to the search keywords */
    Q_FOREACH(const Accounts::Provider &provider, manager->providerList()) {
        keywords.append(provider.name().toLower());
        keywords.append(translations(provider.displayName(),
                                     provider.trCatalog()));
    }

    /* Same for services */
    Q_FOREACH(const Accounts::Service &service, manager->serviceList()) {
        keywords.append(service.name().toLower());
        keywords.append(translations(service.displayName(),
                                     service.trCatalog()));
    }

    /* Also add the account display names */
    Q_FOREACH(Accounts::AccountId id, manager->accountList()) {
        Accounts::Account *account = manager->account(id);
        if (Q_UNLIKELY(!account)) continue;
        keywords.append(account->displayName().toLower());
    }

    delete manager;

    setKeywords(keywords);
}

ItemBase *Plugin::createItem(const QVariantMap &staticData,
                             QObject *parent)
{
    return new Item(staticData, parent);
}

#include "plugin.moc"

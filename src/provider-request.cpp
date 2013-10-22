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

#include "account-manager.h"
#include "application-manager.h"
#include "debug.h"
#include "globals.h"
#include "provider-request.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

class ProviderRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(ProviderRequest)

public:
    ProviderRequestPrivate(ProviderRequest *request);
    ~ProviderRequestPrivate();

    void start();

private Q_SLOTS:
    void onWindowVisibleChanged(bool visible);
    void onDenied();
    void onAllowed(int accountId);

private:
    QVariantMap providerInfo(const QString &providerId) const;

private:
    mutable ProviderRequest *q_ptr;
    QQuickView *m_view;
    QVariantMap m_applicationInfo;
};

} // namespace

ProviderRequestPrivate::ProviderRequestPrivate(ProviderRequest *request):
    QObject(request),
    q_ptr(request),
    m_view(0)
{
}

ProviderRequestPrivate::~ProviderRequestPrivate()
{
    delete m_view;
}

void ProviderRequestPrivate::start()
{
    Q_Q(ProviderRequest);

    QString applicationId =
        q->parameters().value(OAU_KEY_APPLICATION).toString();
    ApplicationManager *appManager = ApplicationManager::instance();
    m_applicationInfo =
        appManager->applicationInfo(applicationId,
                                    q->clientApparmorProfile());
    if (Q_UNLIKELY(m_applicationInfo.isEmpty())) {
        q->fail(OAU_ERROR_INVALID_APPLICATION,
                QStringLiteral("Invalid client application"));
        return;
    }

    QString providerId = q->parameters().value(OAU_KEY_PROVIDER).toString();
    QVariantMap provider = providerInfo(providerId);

    m_view = new QQuickView;
    QObject::connect(m_view, SIGNAL(visibleChanged(bool)),
                     this, SLOT(onWindowVisibleChanged(bool)));
    m_view->setResizeMode(QQuickView::SizeRootObjectToView);
    m_view->engine()->addImportPath(PLUGIN_PRIVATE_MODULE_DIR);

    QQmlContext *context = m_view->rootContext();

    context->setContextProperty("qmlPluginPath",
                                QUrl::fromLocalFile(OAU_PLUGIN_DIR));
    context->setContextProperty("provider", provider);
    context->setContextProperty("application", m_applicationInfo);
    context->setContextProperty("mainWindow", m_view);

    m_view->setSource(QUrl(QStringLiteral("qrc:/qml/ProviderRequest.qml")));
    QQuickItem *root = m_view->rootObject();
    QObject::connect(root, SIGNAL(denied()),
                     this, SLOT(onDenied()));
    QObject::connect(root, SIGNAL(allowed(int)),
                     this, SLOT(onAllowed(int)));
    q->setWindow(m_view);
}

QVariantMap
ProviderRequestPrivate::providerInfo(const QString &providerId) const
{
    Accounts::Provider provider =
        AccountManager::instance()->provider(providerId);

    QVariantMap info;
    info.insert(QStringLiteral("id"), providerId);
    info.insert(QStringLiteral("displayName"), provider.displayName());
    info.insert(QStringLiteral("icon"), provider.iconName());
    return info;
}

void ProviderRequestPrivate::onWindowVisibleChanged(bool visible)
{
    Q_Q(ProviderRequest);

    DEBUG() << visible;

    if (!visible) {
        q->setResult(QVariantMap());
    }
}

void ProviderRequestPrivate::onDenied()
{
    DEBUG();
    /* Just close the window; this will deliver the empty result to the
     * client */
    m_view->close();
}

void ProviderRequestPrivate::onAllowed(int accountId)
{
    Q_Q(ProviderRequest);
    DEBUG() << "Access allowed for account:" << accountId;
    QVariantMap result;
    result.insert("accountId", quint32(accountId));
    q->setResult(result);
    m_view->close();
}

ProviderRequest::ProviderRequest(const QDBusConnection &connection,
                                 const QDBusMessage &message,
                                 const QVariantMap &parameters,
                                 QObject *parent):
    Request(connection, message, parameters, parent),
    d_ptr(new ProviderRequestPrivate(this))
{
}

ProviderRequest::~ProviderRequest()
{
}

void ProviderRequest::start()
{
    Q_D(ProviderRequest);
    Request::start();
    d->start();
}

#include "provider-request.moc"

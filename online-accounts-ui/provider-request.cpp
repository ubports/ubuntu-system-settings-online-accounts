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

#include "access-model.h"
#include "debug.h"
#include "globals.h"
#include "provider-request.h"

#include <OnlineAccountsPlugin/account-manager.h>
#include <OnlineAccountsPlugin/application-manager.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardPaths>

using namespace OnlineAccountsUi;

static bool firstTime = true;

namespace OnlineAccountsUi {

class ProviderRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(ProviderRequest)
    Q_PROPERTY(QVariantMap application READ applicationInfo CONSTANT)
    Q_PROPERTY(QVariantMap provider READ providerInfo CONSTANT)

public:
    ProviderRequestPrivate(ProviderRequest *request);
    ~ProviderRequestPrivate();

    void start();

    QVariantMap applicationInfo() const { return m_applicationInfo; }
    QVariantMap providerInfo() const { return m_providerInfo; }

public Q_SLOTS:
    void deny();
    void allow(int accountId);

private Q_SLOTS:
    void onWindowVisibleChanged(bool visible);

private:
    mutable ProviderRequest *q_ptr;
    QQuickView *m_view;
    QVariantMap m_applicationInfo;
    QVariantMap m_providerInfo;
};

} // namespace

ProviderRequestPrivate::ProviderRequestPrivate(ProviderRequest *request):
    QObject(request),
    q_ptr(request),
    m_view(0)
{
    if (firstTime) {
        qmlRegisterType<QAbstractItemModel>();
        qmlRegisterType<AccessModel>("Ubuntu.OnlineAccounts.Internal",
                                     1, 0, "AccessModel");
        firstTime = false;
    }
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

    QString providerId;
    QString serviceId = q->parameters().value(OAU_KEY_SERVICE_ID).toString();
    if (!serviceId.isEmpty()) {
        Accounts::Service service =
            AccountManager::instance()->service(serviceId);
        if (Q_UNLIKELY(!service.isValid())) {
            q->fail(OAU_ERROR_INVALID_SERVICE,
                    QString("Service %1 not found").arg(serviceId));
            return;
        }
        providerId = service.provider();
    } else {
        providerId = q->parameters().value(OAU_KEY_PROVIDER).toString();
    }
    m_providerInfo = appManager->providerInfo(providerId);

    m_view = new QQuickView;
    QObject::connect(m_view, SIGNAL(visibleChanged(bool)),
                     this, SLOT(onWindowVisibleChanged(bool)));
    m_view->setResizeMode(QQuickView::SizeRootObjectToView);
    QString mountPoint = q->mountPoint();
    QQmlEngine *engine = m_view->engine();
    engine->addImportPath(mountPoint + PLUGIN_PRIVATE_MODULE_DIR);

    /* If the plugin comes from a click package, also add
     *   <package-dir>/lib
     *   <package-dir>/lib/<DEB_HOST_MULTIARCH>
     * to the QML import path.
     */
    QString packageDir = m_providerInfo.value("package-dir").toString();
    if (!packageDir.isEmpty()) {
        engine->addImportPath(packageDir + "/lib");
#ifdef DEB_HOST_MULTIARCH
        engine->addImportPath(packageDir + "/lib/" DEB_HOST_MULTIARCH);
#endif
    }

    QQmlContext *context = m_view->rootContext();

    context->setContextProperty("systemQmlPluginPath",
                                QUrl::fromLocalFile(mountPoint + OAU_PLUGIN_DIR));
    context->setContextProperty("localQmlPluginPath",
                                QUrl::fromLocalFile(QStandardPaths::writableLocation(
                                    QStandardPaths::GenericDataLocation) +
                                "/accounts/qml-plugins/"));
    context->setContextProperty("request", this);
    context->setContextProperty("mainWindow", m_view);

    m_view->setSource(QUrl(QStringLiteral("qrc:/qml/ProviderRequest.qml")));
    /* It could be that allow() or deny() have been already called; don't show
     * the window in that case. */
    if (q->isInProgress()) {
        q->setWindow(m_view);
    }
}

void ProviderRequestPrivate::onWindowVisibleChanged(bool visible)
{
    Q_Q(ProviderRequest);

    QObject::disconnect(m_view, SIGNAL(visibleChanged(bool)),
                        this, SLOT(onWindowVisibleChanged(bool)));
    if (!visible) {
        q->setResult(QVariantMap());
    }
}

void ProviderRequestPrivate::deny()
{
    Q_Q(ProviderRequest);
    DEBUG();
    if (m_view->isVisible()) {
        /* Just close the window; this will deliver the empty result to the
         * client */
        m_view->close();
    } else {
        q->setResult(QVariantMap());
    }
}

void ProviderRequestPrivate::allow(int accountId)
{
    Q_Q(ProviderRequest);
    DEBUG() << "Access allowed for account:" << accountId;
    /* If the request came from an app, add a small delay so that we could
     * serve an authentication request coming right after this one. */
    if (m_view->isVisible() &&
        m_applicationInfo.value("id").toString() != "system-settings") {
        q->setDelay(3000);
    }
    QVariantMap result;
    result.insert(OAU_KEY_ACCOUNT_ID, quint32(accountId));
    q->setResult(result);
    /* We keep the view opened */
}

ProviderRequest::ProviderRequest(const QString &interface,
                                 int id,
                                 const QString &clientProfile,
                                 const QVariantMap &parameters,
                                 QObject *parent):
    Request(interface, id, clientProfile, parameters, parent),
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

/*
 * This file is part of online-accounts-ui
 *
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

#include "browser-request.h"

#include "debug.h"
#include "dialog.h"
#include "globals.h"
#include "i18n.h"

#include <OnlineAccountsPlugin/request-handler.h>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QQmlContext>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QTimer>
#include <QVariant>
#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

#ifndef SIGNONUI_FAIL_TIMEOUT
#define SIGNONUI_FAIL_TIMEOUT 3000
#endif

namespace SignOnUi {

class BrowserRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(BrowserRequest)
    Q_PROPERTY(QUrl pageComponentUrl READ pageComponentUrl CONSTANT)
    Q_PROPERTY(QUrl currentUrl READ currentUrl WRITE setCurrentUrl)
    Q_PROPERTY(QUrl startUrl READ startUrl CONSTANT)
    Q_PROPERTY(QUrl finalUrl READ finalUrl CONSTANT)
    Q_PROPERTY(QUrl rootDir READ rootDir CONSTANT)

public:
    BrowserRequestPrivate(BrowserRequest *request);
    ~BrowserRequestPrivate();

    void start();

    void setCurrentUrl(const QUrl &url);
    QUrl pageComponentUrl() const;
    QUrl currentUrl() const { return m_currentUrl; }
    QUrl startUrl() const { return m_startUrl; }
    QUrl finalUrl() const { return m_finalUrl; }
    QUrl responseUrl() const { return m_responseUrl; }
    QUrl rootDir() const { return QUrl::fromLocalFile(m_rootDir); }

    QString rootDirForIdentity() const;

public Q_SLOTS:
    void setCookies(const QVariant &cookies);
    void cancel();
    void onLoadStarted();
    void onLoadFinished(bool ok);

private Q_SLOTS:
    void onFailTimer();
    void onFinished();

Q_SIGNALS:
    void authenticated();

private:
    void buildDialog(const QVariantMap &params);
    void closeView();

private:
    Dialog *m_dialog;
    QUrl m_currentUrl;
    QUrl m_startUrl;
    QUrl m_finalUrl;
    QUrl m_responseUrl;
    QString m_rootDir;
    QTimer m_failTimer;
    mutable BrowserRequest *q_ptr;
};

} // namespace

BrowserRequestPrivate::BrowserRequestPrivate(BrowserRequest *request):
    QObject(request),
    m_dialog(0),
    q_ptr(request)
{
    m_failTimer.setSingleShot(true);
    m_failTimer.setInterval(SIGNONUI_FAIL_TIMEOUT);
    QObject::connect(&m_failTimer, SIGNAL(timeout()),
                     this, SLOT(onFailTimer()));
}

BrowserRequestPrivate::~BrowserRequestPrivate()
{
    DEBUG();
    closeView();
    delete m_dialog;
}

QString BrowserRequestPrivate::rootDirForIdentity() const
{
    Q_Q(const BrowserRequest);
    QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return cachePath + QString("/id-%1-%2").arg(q->identity()).arg(q->providerId());
}

void BrowserRequestPrivate::start()
{
    Q_Q(BrowserRequest);

    const QVariantMap &params = q->parameters();
    DEBUG() << params;

    QDir rootDir(rootDirForIdentity());
    if (!rootDir.exists()) {
        rootDir.mkpath(".");
    }

    m_finalUrl = params.value(SSOUI_KEY_FINALURL).toString();
    m_startUrl = params.value(SSOUI_KEY_OPENURL).toString();
    m_rootDir = rootDir.absolutePath();
    if (!q->hasHandler()) {
        buildDialog(params);

        QObject::connect(m_dialog, SIGNAL(finished(int)),
                         this, SLOT(onFinished()));

        m_dialog->engine()->addImportPath(PLUGIN_PRIVATE_MODULE_DIR);
        m_dialog->rootContext()->setContextProperty("request", this);
        m_dialog->setSource(QUrl("qrc:/qml/SignOnUiPage.qml"));
    } else {
        DEBUG() << "Setting request on handler";
        q->handler()->setRequest(this);
    }
}

QUrl BrowserRequestPrivate::pageComponentUrl() const
{
    Q_Q(const BrowserRequest);
    /* We define the X-PageComponent key to let the clients override the QML
     * component to be used to build the authentication page.
     * To prevent a malicious client to show it's own UI, we require that the
     * file path begins with "/usr/share/signon-ui/" (where Ubuntu click
     * packages cannot install files).
     */
    QUrl providedUrl = q->clientData().value("X-PageComponent").toString();
    if (providedUrl.isValid() && providedUrl.isLocalFile() &&
        providedUrl.path().startsWith("/usr/share/signon-ui/")) {
        return providedUrl;
    } else {
        return QStringLiteral("DefaultPage.qml");
    }
}

void BrowserRequestPrivate::setCurrentUrl(const QUrl &url)
{
    DEBUG() << "Url changed:" << url;
    if (m_failTimer.isActive()) {
        m_failTimer.start();
    }

    m_currentUrl = url;
    if (url.host() == m_finalUrl.host() &&
        url.path() == m_finalUrl.path()) {
        m_responseUrl = url;
        if (m_dialog != 0) {
            if (!m_dialog->isVisible()) {
                /* Do not show the notification page. */
                m_dialog->accept();
            } else {
                /* Replace the web page with an information screen */
                /* TODO */
                Q_EMIT authenticated();
            }
        } else {
            DEBUG();
            Q_EMIT authenticated();
        }
    }
}

void BrowserRequestPrivate::setCookies(const QVariant &cookies)
{
    DEBUG() << cookies;

    QJsonArray jsonCookies =
        QJsonArray::fromVariantList(cookies.toList());

    /* Save the cookies into a text file */
    QFile file(m_rootDir + "/cookies.json");
    if (Q_LIKELY(file.open(QIODevice::WriteOnly | QIODevice::Text))) {
        QJsonDocument doc(jsonCookies);
        file.write(doc.toJson());
        file.close();
    }

    onFinished();
}

void BrowserRequestPrivate::cancel()
{
    Q_Q(BrowserRequest);

    DEBUG() << "Client requested to cancel";
    q->setCanceled();
    closeView();
}

void BrowserRequestPrivate::onLoadStarted()
{
    m_failTimer.stop();
}

void BrowserRequestPrivate::onLoadFinished(bool ok)
{
    Q_Q(BrowserRequest);

    DEBUG() << "Load finished" << ok;

    if (!ok) {
        m_failTimer.start();
        return;
    }

    if (m_dialog && !m_dialog->isVisible()) {
        if (m_responseUrl.isEmpty()) {
            q->setWindow(m_dialog);
        } else {
            Q_EMIT authenticated();
        }
    }
}

void BrowserRequestPrivate::onFailTimer()
{
    Q_Q(BrowserRequest);

    DEBUG() << "Page loading failed";
    closeView();
    QVariantMap result;
    result[SSOUI_KEY_ERROR] = SignOn::QUERY_ERROR_NETWORK;
    q->setResult(result);
}

void BrowserRequestPrivate::onFinished()
{
    Q_Q(BrowserRequest);

    DEBUG() << "Browser dialog closed";
    QObject::disconnect(m_dialog, SIGNAL(finished(int)),
                        this, SLOT(onFinished()));

    QVariantMap reply;
    QUrl url = m_responseUrl.isEmpty() ? m_currentUrl : m_responseUrl;
    reply[SSOUI_KEY_URLRESPONSE] = url.toString();

    closeView();

    q->setResult(reply);
}

void BrowserRequestPrivate::buildDialog(const QVariantMap &params)
{
    m_dialog = new Dialog;

    QString title;
    if (params.contains(SSOUI_KEY_TITLE)) {
        title = params[SSOUI_KEY_TITLE].toString();
    } else if (params.contains(SSOUI_KEY_CAPTION)) {
        title = OnlineAccountsUi::_("Web authentication for %1",
                                    SIGNONUI_I18N_DOMAIN).
            arg(params[SSOUI_KEY_CAPTION].toString());
    } else {
        title = OnlineAccountsUi::_("Web authentication",
                                    SIGNONUI_I18N_DOMAIN);
    }

    m_dialog->setTitle(title);

    DEBUG() << "Dialog was built";
}

void BrowserRequestPrivate::closeView()
{
    Q_Q(BrowserRequest);

    DEBUG();
    if (q->hasHandler()) {
        q->handler()->setRequest(0);
    } else if (m_dialog) {
        m_dialog->close();
    }
}

BrowserRequest::BrowserRequest(int id,
                               const QString &clientProfile,
                               const QVariantMap &parameters,
                               QObject *parent):
    Request(id, clientProfile, parameters, parent),
    d_ptr(new BrowserRequestPrivate(this))
{
}

BrowserRequest::~BrowserRequest()
{
    delete d_ptr;
}

void BrowserRequest::start()
{
    Q_D(BrowserRequest);

    Request::start();
    d->start();
}

#include "browser-request.moc"

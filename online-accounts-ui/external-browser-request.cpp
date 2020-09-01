/*
 * This file is part of online-accounts-ui
 *
 * Copyright (C) 2021 UBports Foundation
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
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

#include "external-browser-request.h"

#include "debug.h"
#include "dialog.h"
#include "globals.h"
#include "i18n.h"

#include <OnlineAccountsPlugin/request-handler.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QVariant>
#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

namespace SignOnUi {

class ExternalBrowserRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(ExternalBrowserRequest)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QUrl startUrl READ startUrl CONSTANT)

public:
    ExternalBrowserRequestPrivate(ExternalBrowserRequest *request);
    ~ExternalBrowserRequestPrivate();

    void start();

    QString title() const { return q_ptr->windowTitle(); }
    QUrl startUrl() const { return m_startUrl; }

public Q_SLOTS:
    /* This method must be called when the final URL has been visited.
     * Typically, a client should have setup a LoopbackServer on localhost, and
     * forward the URL from the LoopbackServer::visited() signal to this slot.
     */
    void urlVisited(const QUrl &url);
    void cancel();

private Q_SLOTS:
    void onFinished();

private:
    void buildDialog(const QVariantMap &params);
    void closeView();

private:
    Dialog *m_dialog;
    QUrl m_startUrl;
    QUrl m_responseUrl;
    mutable ExternalBrowserRequest *q_ptr;
};

} // namespace

ExternalBrowserRequestPrivate::ExternalBrowserRequestPrivate(ExternalBrowserRequest *request):
    QObject(request),
    m_dialog(0),
    q_ptr(request)
{
}

ExternalBrowserRequestPrivate::~ExternalBrowserRequestPrivate()
{
    DEBUG();
    closeView();
    delete m_dialog;
}

void ExternalBrowserRequestPrivate::start()
{
    Q_Q(ExternalBrowserRequest);

    const QVariantMap &params = q->parameters();
    DEBUG() << params;

    m_startUrl = params.value(SSOUI_KEY_OPENURL).toString();
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

void ExternalBrowserRequestPrivate::urlVisited(const QUrl &url)
{
    DEBUG() << "Url visited:" << url;

    m_responseUrl = url;
    onFinished();
}

void ExternalBrowserRequestPrivate::cancel()
{
    Q_Q(ExternalBrowserRequest);

    DEBUG() << "Client requested to cancel";
    q->setCanceled();
    closeView();
}

void ExternalBrowserRequestPrivate::onFinished()
{
    Q_Q(ExternalBrowserRequest);

    DEBUG() << "ExternalBrowser dialog closed";
    QObject::disconnect(m_dialog, SIGNAL(finished(int)),
                        this, SLOT(onFinished()));

    QVariantMap reply;
    reply[SSOUI_KEY_URLRESPONSE] = m_responseUrl.toString();

    closeView();

    q->setResult(reply);
}

void ExternalBrowserRequestPrivate::buildDialog(const QVariantMap &params)
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

void ExternalBrowserRequestPrivate::closeView()
{
    Q_Q(ExternalBrowserRequest);

    DEBUG();
    if (q->hasHandler()) {
        q->handler()->setRequest(0);
    } else if (m_dialog) {
        m_dialog->close();
    }
}

ExternalBrowserRequest::ExternalBrowserRequest(
        int id,
        const QString &clientProfile,
        const QVariantMap &parameters,
        QObject *parent):
    Request(id, clientProfile, parameters, parent),
    d_ptr(new ExternalBrowserRequestPrivate(this))
{
}

ExternalBrowserRequest::~ExternalBrowserRequest()
{
}

void ExternalBrowserRequest::start()
{
    Q_D(ExternalBrowserRequest);

    Request::start();
    d->start();
}

#include "external-browser-request.moc"

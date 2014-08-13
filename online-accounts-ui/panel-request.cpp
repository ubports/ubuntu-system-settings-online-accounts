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

#include "application-manager.h"
#include "debug.h"
#include "globals.h"
#include "panel-request.h"

#include <QDesktopServices>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

class PanelRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(PanelRequest)

public:
    PanelRequestPrivate(PanelRequest *request);
    ~PanelRequestPrivate();

    void start();

private Q_SLOTS:
    void onWindowVisibleChanged(bool visible);

private:
    mutable PanelRequest *q_ptr;
    QQuickView *m_view;
};

} // namespace

PanelRequestPrivate::PanelRequestPrivate(PanelRequest *request):
    QObject(request),
    q_ptr(request),
    m_view(0)
{
}

PanelRequestPrivate::~PanelRequestPrivate()
{
    DEBUG() << "view:" << m_view;
    /* TODO Uncomment this once QTBUG-40766 is resolved:
    delete m_view;
    */
}

void PanelRequestPrivate::start()
{
    Q_Q(PanelRequest);

    m_view = new QQuickView;
    QObject::connect(m_view, SIGNAL(visibleChanged(bool)),
                     this, SLOT(onWindowVisibleChanged(bool)));
    m_view->setResizeMode(QQuickView::SizeRootObjectToView);
    m_view->engine()->addImportPath(PLUGIN_PRIVATE_MODULE_DIR);

    QQmlContext *context = m_view->rootContext();

    context->setContextProperty("systemQmlPluginPath",
                                QUrl::fromLocalFile(OAU_PLUGIN_DIR));
    context->setContextProperty("localQmlPluginPath",
                                QUrl::fromLocalFile(QStandardPaths::writableLocation(
                                    QStandardPaths::GenericDataLocation) +
                                "/accounts/qml-plugins/"));
    context->setContextProperty("pluginOptions", QVariantMap());
    context->setContextProperty("mainWindow", m_view);
    context->setContextProperty("ApplicationManager",
                                ApplicationManager::instance());

    m_view->setSource(QUrl(QStringLiteral("qrc:/qml/MainPage.qml")));
    q->setWindow(m_view);
}

void PanelRequestPrivate::onWindowVisibleChanged(bool visible)
{
    Q_Q(PanelRequest);

    DEBUG() << visible;

    if (!visible) {
        q->setResult(QVariantMap());
    }
}

PanelRequest::PanelRequest(const QString &interface,
                           int id,
                           const QString &clientProfile,
                           const QVariantMap &parameters,
                           QObject *parent):
    Request(interface, id, clientProfile, parameters, parent),
    d_ptr(new PanelRequestPrivate(this))
{
}

PanelRequest::~PanelRequest()
{
}

void PanelRequest::start()
{
    Q_D(PanelRequest);
    Request::start();
    d->start();
}

#include "panel-request.moc"

/*
 * This file is part of online-accounts-ui
 *
 * Copyright (C) 2014 Canonical Ltd.
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

#include "dialog-request.h"

#include "debug.h"
#include "dialog.h"
#include "globals.h"
#include "i18n.h"

#include <OnlineAccountsPlugin/request-handler.h>
#include <QDir>
#include <QQmlContext>
#include <QQmlEngine>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

/* These fields are temporarily defined here; they'll be eventually moved to
 * signond's include files. */
#define SSOUI_KEY_USERNAME_TEXT QLatin1String("UserNameText")
#define SSOUI_KEY_PASSWORD_TEXT QLatin1String("PasswordText")
#define SSOUI_KEY_REGISTER_URL  QLatin1String("RegisterUrl")
#define SSOUI_KEY_REGISTER_TEXT QLatin1String("RegisterText")
#define SSOUI_KEY_LOGIN_TEXT QLatin1String("LoginText")

namespace SignOnUi {

class DialogRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(DialogRequest)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString userName READ userName WRITE setUserName \
               NOTIFY userNameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword \
               NOTIFY passwordChanged)
    Q_PROPERTY(QString userNameText READ userNameText CONSTANT)
    Q_PROPERTY(QString passwordText READ passwordText CONSTANT)
    Q_PROPERTY(QString message READ message CONSTANT)
    Q_PROPERTY(bool queryUserName READ queryUserName CONSTANT)
    Q_PROPERTY(bool queryPassword READ queryPassword CONSTANT)
    Q_PROPERTY(QUrl forgotPasswordUrl READ forgotPasswordUrl CONSTANT)
    Q_PROPERTY(QString forgotPasswordText READ forgotPasswordText CONSTANT)
    Q_PROPERTY(QUrl registerUrl READ registerUrl CONSTANT)
    Q_PROPERTY(QString registerText READ registerText CONSTANT)
    Q_PROPERTY(QString loginText READ loginText CONSTANT)

public:
    DialogRequestPrivate(DialogRequest *request);
    ~DialogRequestPrivate();

    void start();

    QString title() const { return m_title; }
    void setUserName(const QString &userName);
    QString userName() const { return m_userName; }
    void setPassword(const QString &password);
    QString password() const { return m_password; }
    QString userNameText() const { return m_userNameText; }
    QString passwordText() const { return m_passwordText; }
    QString message() const { return m_message; }
    bool queryUserName() const { return m_queryUsername; }
    bool queryPassword() const { return m_queryPassword; }
    QUrl forgotPasswordUrl() const { return m_forgotPasswordUrl; }
    QString forgotPasswordText() const { return m_forgotPasswordText; }
    QUrl registerUrl() const { return m_registerUrl; }
    QString registerText() const { return m_registerText; }
    QString loginText() const { return m_loginText; }

public Q_SLOTS:
    void cancel();
    void accept();

Q_SIGNALS:
    void userNameChanged();
    void passwordChanged();

private Q_SLOTS:
    void onFinished();

private:
    void closeView();

private:
    Dialog *m_dialog;
    QString m_title;
    QString m_userName;
    QString m_password;
    QString m_userNameText;
    QString m_passwordText;
    QString m_message;
    bool m_queryUsername;
    bool m_queryPassword;
    QUrl m_forgotPasswordUrl;
    QString m_forgotPasswordText;
    QUrl m_registerUrl;
    QString m_registerText;
    QString m_loginText;
    mutable DialogRequest *q_ptr;
};

} // namespace

DialogRequestPrivate::DialogRequestPrivate(DialogRequest *request):
    QObject(request),
    m_dialog(0),
    m_queryUsername(false),
    m_queryPassword(false),
    q_ptr(request)
{
    const QVariantMap &params = q_ptr->parameters();

    if (params.contains(SSOUI_KEY_TITLE)) {
        m_title = params[SSOUI_KEY_TITLE].toString();
    } else if (params.contains(SSOUI_KEY_CAPTION)) {
        m_title = OnlineAccountsUi::_("Web authentication for %1",
                                      SIGNONUI_I18N_DOMAIN).
            arg(params[SSOUI_KEY_CAPTION].toString());
    } else {
        m_title = OnlineAccountsUi::_("Web authentication",
                                      SIGNONUI_I18N_DOMAIN);
    }

    m_queryUsername = params.value(SSOUI_KEY_QUERYUSERNAME, false).toBool();
    m_userName = params.value(SSOUI_KEY_USERNAME).toString();
    m_userNameText = params.value(SSOUI_KEY_USERNAME_TEXT).toString();
    if (m_userNameText.isEmpty()) {
        m_userNameText = OnlineAccountsUi::_("Username:",
                                             SIGNONUI_I18N_DOMAIN);
    }

    m_queryPassword = params.value(SSOUI_KEY_QUERYPASSWORD, false).toBool();
    m_password = params.value(SSOUI_KEY_PASSWORD).toString();
    m_passwordText = params.value(SSOUI_KEY_PASSWORD_TEXT).toString();
    if (m_passwordText.isEmpty()) {
        m_passwordText = OnlineAccountsUi::_("Password:",
                                             SIGNONUI_I18N_DOMAIN);
    }


    m_message = params.value(SSOUI_KEY_MESSAGE).toString();

    m_forgotPasswordUrl =
        QUrl(params.value(SSOUI_KEY_FORGOTPASSWORDURL).toString());
    m_forgotPasswordText = params.value(SSOUI_KEY_FORGOTPASSWORD).toString();

    m_registerUrl =
        QUrl(params.value(SSOUI_KEY_REGISTER_URL).toString());
    m_registerText = params.value(SSOUI_KEY_REGISTER_TEXT).toString();

    m_loginText = params.value(SSOUI_KEY_LOGIN_TEXT).toString();
    if (m_loginText.isEmpty()) {
        m_loginText = OnlineAccountsUi::_("Sign in");
    }
}

DialogRequestPrivate::~DialogRequestPrivate()
{
    closeView();
    delete m_dialog;
}

void DialogRequestPrivate::start()
{
    Q_Q(DialogRequest);

    const QVariantMap &params = q->parameters();
    DEBUG() << params;

    if (!q->hasHandler()) {
        m_dialog = new Dialog;
        m_dialog->setTitle(m_title);

        QObject::connect(m_dialog, SIGNAL(finished(int)),
                         this, SLOT(onFinished()));

        m_dialog->engine()->addImportPath(PLUGIN_PRIVATE_MODULE_DIR);
        m_dialog->rootContext()->setContextProperty("request", this);
        m_dialog->setSource(QUrl("qrc:/qml/SignOnUiDialog.qml"));
        q->setWindow(m_dialog);
    } else {
        DEBUG() << "Setting request on handler";
        q->handler()->setRequest(this);
    }
}

void DialogRequestPrivate::accept()
{
    DEBUG() << "User accepted";
    onFinished();
}

void DialogRequestPrivate::cancel()
{
    Q_Q(DialogRequest);

    DEBUG() << "User requested to cancel";
    q->setCanceled();
    closeView();
}

void DialogRequestPrivate::setUserName(const QString &userName)
{
    if (userName == m_userName) return;
    m_userName = userName;
    Q_EMIT userNameChanged();
}

void DialogRequestPrivate::setPassword(const QString &password)
{
    if (password == m_password) return;
    m_password = password;
    Q_EMIT passwordChanged();
}

void DialogRequestPrivate::onFinished()
{
    Q_Q(DialogRequest);

    DEBUG() << "Dialog closed";

    QVariantMap reply;

    if (m_queryUsername) {
        reply[SSOUI_KEY_USERNAME] = m_userName;
    }
    if (m_queryPassword) {
        reply[SSOUI_KEY_PASSWORD] = m_password;
    }

    closeView();

    q->setResult(reply);
}

void DialogRequestPrivate::closeView()
{
    Q_Q(DialogRequest);

    if (q->hasHandler()) {
        q->handler()->setRequest(0);
    } else if (m_dialog) {
        m_dialog->close();
    }
}

DialogRequest::DialogRequest(int id,
                             const QString &clientProfile,
                             const QVariantMap &parameters,
                             QObject *parent):
    Request(id, clientProfile, parameters, parent),
    d_ptr(new DialogRequestPrivate(this))
{
}

DialogRequest::~DialogRequest()
{
}

void DialogRequest::start()
{
    Q_D(DialogRequest);

    Request::start();
    d->start();
}

#include "dialog-request.moc"

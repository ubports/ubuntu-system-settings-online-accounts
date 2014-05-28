/*
 * This file is part of online-accounts-ui
 *
 * Copyright (C) 2012 Canonical Ltd.
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

#include "indicator-service.h"

#include "debug.h"
#include "i18n.h"
#include "reauthenticator.h"
#include "webcredentials_adaptor.h"

#include <QByteArray>
#include <QDBusContext>
#undef signals
#include <libnotify/notification.h>
#include <libnotify/notify.h>

using namespace OnlineAccountsUi;
using namespace SignOnUi;

QDBusArgument &operator<<(QDBusArgument &argument, const QSet<uint> &set)
{
    argument.beginArray(qMetaTypeId<uint>());
    foreach (uint id, set) {
        argument << id;
    }
    argument.endArray();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                QSet<uint> &set)
{
    argument.beginArray();
    while (!argument.atEnd()) {
        uint id;
        argument >> id;
        set.insert(id);
    }
    argument.endArray();
    return argument;
}

namespace SignOnUi {

static IndicatorService *m_instance = 0;

class IndicatorServicePrivate: public QObject, QDBusContext
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(IndicatorService)

public:
    Q_PROPERTY(QSet<uint> Failures READ failures)
    Q_PROPERTY(bool ErrorStatus READ errorStatus)

    IndicatorServicePrivate(IndicatorService *service);
    ~IndicatorServicePrivate() {};

    QSet<uint> failures() const { return m_failures; }
    bool errorStatus() const { return m_errorStatus; }

public Q_SLOTS:
    void ClearErrorStatus();
    void RemoveFailures(const QSet<uint> &accountIds);
    void ReportFailure(uint accountId, const QVariantMap &notification);
    bool ReauthenticateAccount(uint accountId,
                               const QVariantMap &extraParameters);

private:
    void showNotification(const QVariantMap &parameters);
    void notifyPropertyChanged(const char *propertyName);

private Q_SLOTS:
    void onReauthenticatorFinished(bool success);

private:
    mutable IndicatorService *q_ptr;
    WebcredentialsAdaptor *m_adaptor;
    QSet<uint> m_failures;
    QMap<uint, QList<AuthData> > m_failureClientData;
    QMap<uint, Reauthenticator*> m_reauthenticators;
    QDBusMessage m_clientMessage;
    bool m_errorStatus;
};

} // namespace

IndicatorServicePrivate::IndicatorServicePrivate(IndicatorService *service):
    QObject(service),
    q_ptr(service),
    m_adaptor(new WebcredentialsAdaptor(this)),
    m_errorStatus(false)
{
    qDBusRegisterMetaType< QSet<uint> >();
    notify_init("webcredentials-indicator");
}

void IndicatorServicePrivate::ClearErrorStatus()
{
    if (m_errorStatus) {
        m_errorStatus = false;
        notifyPropertyChanged("ErrorStatus");
    }
}

void IndicatorServicePrivate::RemoveFailures(const QSet<uint> &accountIds)
{
    Q_Q(IndicatorService);
    m_failures.subtract(accountIds);
    notifyPropertyChanged("Failures");
    if (q->isIdle()) {
        Q_EMIT q->isIdleChanged();
    }
}

void IndicatorServicePrivate::ReportFailure(uint accountId,
                                            const QVariantMap &notification)
{
    Q_Q(IndicatorService);
    bool wasIdle = q->isIdle();
    m_failures.insert(accountId);
    if (wasIdle) {
        Q_EMIT q->isIdleChanged();
    }

    /* If the original client data is provided, we remember it: it can
     * be used to replay the authentication later.
     */
    if (notification.contains("ClientData")) {
        /* If the key is not found, the QMap's [] operator inserts an empty
         * element in the map and return a reference to it. So the following
         * line of code returns a valid QList even if the account never failed
         * before.
         */
        QList<AuthData> &failedAuthentications =
            m_failureClientData[accountId];

        AuthData authData;
        authData.sessionData = notification["ClientData"].toMap();
        authData.identity = quint32(notification["Identity"].toUInt());
        authData.method = notification["Method"].toString();
        authData.mechanism = notification["Mechanism"].toString();
        failedAuthentications.append(authData);
    }

    notifyPropertyChanged("Failures");

    showNotification(notification);
}

bool IndicatorServicePrivate::ReauthenticateAccount(uint accountId,
                                   const QVariantMap &extraParameters)
{
    if (!m_failureClientData.contains(accountId)) {
        /* Nothing we can do about this account */
        DEBUG() << "No reauthentication data for account" << accountId;
        return false;
    }

    if (m_reauthenticators.contains(accountId)) {
        /* A reauthenticator for this account is already at work. This
         * shouldn't happen in a real world scenario. */
        qWarning() << "Reauthenticator already active on" << accountId;
        return false;
    }

    DEBUG() << "Reauthenticating account" << accountId;

    /* If we need to reauthenticate, we are delivering the result
     * after iterating the event loop, so we must inform QtDBus that
     * it shouldn't use this method's return value as a result.
     */
    setDelayedReply(true);
    m_clientMessage = message();
    QList<AuthData> &failedAuthentications = m_failureClientData[accountId];

    Reauthenticator *reauthenticator =
        new Reauthenticator(failedAuthentications, extraParameters, this);
    m_reauthenticators[accountId] = reauthenticator;

    QObject::connect(reauthenticator, SIGNAL(finished(bool)),
                     this, SLOT(onReauthenticatorFinished(bool)),
                     Qt::QueuedConnection);
    reauthenticator->start();

    return true; // ignored, see setDelayedReply() above.
}

void IndicatorServicePrivate::showNotification(const QVariantMap &parameters)
{
    /* Don't show more than one notification, until the error status is
     * cleared */
    if (m_errorStatus) return;

    m_errorStatus = true;
    notifyPropertyChanged("ErrorStatus");

    QString applicationName = parameters.value("DisplayName").toString();

    QString summary;
    if (applicationName.isEmpty()) {
        summary = _("Applications can no longer access "
                    "some of your Online Accounts",
                    SIGNONUI_I18N_DOMAIN);
    } else {
        summary = _("Applications can no longer access "
                    "your %1 Online Account",
                    SIGNONUI_I18N_DOMAIN).arg(applicationName);
    }

    QString message = _("Choose <b>Online Accounts</b> from the user "
                        "menu to reinstate access to this account.",
                        SIGNONUI_I18N_DOMAIN);

    QByteArray summaryUtf8 = summary.toUtf8();
    QByteArray messageUtf8 = message.toUtf8();
    NotifyNotification *notification =
        notify_notification_new(summaryUtf8.constData(),
                                messageUtf8.constData(),
                                NULL);

    GError *error = NULL;
    if (!notify_notification_show(notification, &error)) {
        qWarning() << "Couldn't show notification:" << error->message;
        g_clear_error(&error);
    }

    g_object_unref(notification);
}

void IndicatorServicePrivate::notifyPropertyChanged(const char *propertyName)
{
    QDBusMessage signal =
        QDBusMessage::createSignal(WEBCREDENTIALS_OBJECT_PATH,
                                   "org.freedesktop.DBus.Properties",
                                   "PropertiesChanged");
    signal << WEBCREDENTIALS_INTERFACE;
    QVariantMap changedProps;
    changedProps.insert(QString::fromLatin1(propertyName),
                        property(propertyName));
    signal << changedProps;
    signal << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

void IndicatorServicePrivate::onReauthenticatorFinished(bool success)
{
    Q_Q(IndicatorService);

    Reauthenticator *reauthenticator =
        qobject_cast<Reauthenticator*>(sender());

    /* Find the account; searching a map by value is inefficient, but
     * in this case it's extremely likely that the map contains just
     * one element. :-) */
    uint accountId = 0;
    QMap<uint,Reauthenticator*>::const_iterator i;
    for (i = m_reauthenticators.constBegin();
         i != m_reauthenticators.constEnd();
         i++) {
        if (i.value() == reauthenticator) {
            accountId = i.key();
            break;
        }
    }
    Q_ASSERT (accountId != 0);

    QDBusMessage reply = m_clientMessage.createReply(success);
    QDBusConnection::sessionBus().send(reply);

    if (success) {
        m_failureClientData.remove(accountId);
        m_failures.remove(accountId);
        notifyPropertyChanged("Failures");

        if (m_failures.isEmpty()) {
            ClearErrorStatus();
            Q_EMIT q->isIdleChanged();
        }
    }

    m_reauthenticators.remove(accountId);
    reauthenticator->deleteLater();
}

IndicatorService::IndicatorService(QObject *parent):
    QObject(parent),
    d_ptr(new IndicatorServicePrivate(this))
{
    if (m_instance == 0) {
        m_instance = this;
    } else {
        qWarning() << "Instantiating a second IndicatorService!";
    }
}

IndicatorService::~IndicatorService()
{
    m_instance = 0;
    delete d_ptr;
}

IndicatorService *IndicatorService::instance()
{
    return m_instance;
}

QObject *IndicatorService::serviceObject() const
{
    return d_ptr;
}

void IndicatorService::clearErrorStatus()
{
    Q_D(IndicatorService);
    d->ClearErrorStatus();
}

void IndicatorService::removeFailures(const QSet<uint> &accountIds)
{
    Q_D(IndicatorService);
    d->RemoveFailures(accountIds);
}

void IndicatorService::reportFailure(uint accountId,
                                     const QVariantMap &notification)
{
    Q_D(IndicatorService);
    d->ReportFailure(accountId, notification);
}

QSet<uint> IndicatorService::failures() const
{
    Q_D(const IndicatorService);
    return d->m_failures;
}

bool IndicatorService::errorStatus() const
{
    Q_D(const IndicatorService);
    return d->m_errorStatus;
}

bool IndicatorService::isIdle() const
{
    Q_D(const IndicatorService);
    return d->m_failures.isEmpty();
}

#include "indicator-service.moc"

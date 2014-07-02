/*
 * Copyright (C) 2014 Canonical Ltd.
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

#include "notification.h"

#include <QDebug>
#include <QList>
#include <QMap>
#include <QPair>
#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>
#include <libnotify/notification.h>
#include <libnotify/notify.h>

using namespace OnlineAccountsUi;

/* Mocking libnotify: */

static char *appName = NULL;

gboolean notify_init(const char *app_name)
{
    appName = g_strdup(app_name);
    return true;
}

gboolean notify_is_initted()
{
    return appName != NULL;
}

typedef QPair<QByteArray,QByteArray> ActionPair;

struct MockNotification {
    QString summary;
    QString body;
    QList<ActionPair> actions;
    QVariantMap hints;
    bool visible;

    MockNotification();
    ~MockNotification();
    bool invokeAction(const QByteArray &action);
    void close();

    struct CallbackData {
        NotifyActionCallback callback;
        gpointer userData;
    };

    QMap<QByteArray,CallbackData> callbacks;

    /* "closed" signal connection */
    void (*closedCallback)(gpointer, gpointer);
    gpointer closedUserData;
};

static QSet<MockNotification*> notifications;

MockNotification::MockNotification():
    visible(false)
{
    notifications.insert(this);
}

MockNotification::~MockNotification()
{
    notifications.remove(this);
}

bool MockNotification::invokeAction(const QByteArray &action)
{
    if (!callbacks.contains(action)) return false;
    CallbackData &callbackData = callbacks[action];
    callbackData.callback(reinterpret_cast<NotifyNotification*>(this),
                          (char *)action.data(), callbackData.userData);
    return true;
}

void MockNotification::close()
{
    if (!closedCallback) return;

    closedCallback(closedUserData, this);
}

gulong g_signal_connect_data(gpointer instance,
                             const gchar *detailed_signal,
                             GCallback c_handler,
                             gpointer data,
                             GClosureNotify destroy_data,
                             GConnectFlags connect_flags)
{
    Q_UNUSED(destroy_data);
    Q_UNUSED(connect_flags);

    MockNotification *mock = reinterpret_cast<MockNotification*>(instance);
    if (qstrcmp(detailed_signal, "closed") == 0) {
        mock->closedCallback = (void (*)(gpointer,gpointer))c_handler;
        mock->closedUserData = data;
    } else {
        qWarning() << "Unmocked signal" << detailed_signal;
        return 0;
    }

    return 1;
}

void g_object_unref(gpointer object)
{
    MockNotification *mock = reinterpret_cast<MockNotification*>(object);
    delete mock;
}

NotifyNotification *
notify_notification_new(const char *summary,
                        const char *body,
                        const char *icon)
{
    Q_UNUSED(icon);
    MockNotification *notification = new MockNotification;
    notification->summary = QString::fromUtf8(summary);
    notification->body = QString::fromUtf8(body);
    return reinterpret_cast<NotifyNotification*>(notification);
}

gboolean
notify_notification_show(NotifyNotification *notification,
                         GError **error)
{
    Q_UNUSED(error);
    MockNotification *mock =
        reinterpret_cast<MockNotification*>(notification);
    mock->visible = true;
    return true;
}

void notify_notification_add_action(NotifyNotification *notification,
                                    const char *action,
                                    const char *label,
                                    NotifyActionCallback callback,
                                    gpointer user_data,
                                    GFreeFunc free_func)
{
    Q_UNUSED(free_func);
    MockNotification *mock =
        reinterpret_cast<MockNotification*>(notification);
    mock->actions.append(ActionPair(action, label));
    MockNotification::CallbackData &callbackData = mock->callbacks[action];
    callbackData.callback = callback;
    callbackData.userData = user_data;
}

void notify_notification_set_hint(NotifyNotification *notification,
                                  const char *key,
                                  GVariant *value)
{
    MockNotification *mock =
        reinterpret_cast<MockNotification*>(notification);
    QVariant variant;
    if (g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
        variant = bool(g_variant_get_boolean(value));
    } else {
        /* Add support for any needed types */
        qWarning() << "Unsupported variant type";
    }
    mock->hints.insert(QString::fromUtf8(key), variant);
}

/* End of mock code */

class NotificationTest: public QObject
{
    Q_OBJECT

public:
    NotificationTest();

private Q_SLOTS:
    void testInitialization();
    void testDestruction();
    void testContents();
    void testSnapDecision();
    void testVisibility();
    void testClosing();
    void testActions();
};

NotificationTest::NotificationTest():
    QObject(0)
{
}

void NotificationTest::testInitialization()
{
    Notification first("summary", "body");
    QVERIFY(notify_is_initted());
}

void NotificationTest::testDestruction()
{
    QVERIFY(notifications.isEmpty());

    {
        Notification one("one", "One");
        QCOMPARE(notifications.count(), 1);

        {
            Notification two("two", "Two");
            QCOMPARE(notifications.count(), 2);
        }
        QCOMPARE(notifications.count(), 1);
    }

    QCOMPARE(notifications.count(), 0);
}

void NotificationTest::testContents()
{
    Notification first("Summary", "Body");
    MockNotification *mock = *(notifications.begin());
    QCOMPARE(mock->summary, QString("Summary"));
    QCOMPARE(mock->body, QString("Body"));
    QVERIFY(mock->hints.isEmpty());
}

void NotificationTest::testSnapDecision()
{
    Notification first("Summary", "Body");
    MockNotification *mock = *(notifications.begin());
    QVERIFY(mock->hints.isEmpty());

    first.setSnapDecision(true);
    QCOMPARE(mock->hints["x-canonical-snap-decisions"].toBool(), true);

    first.setSnapDecision(false);
    QCOMPARE(mock->hints["x-canonical-snap-decisions"].toBool(), false);
}

void NotificationTest::testVisibility()
{
    Notification first("Summary", "Body");
    MockNotification *mock = *(notifications.begin());
    QVERIFY(!mock->visible);

    first.show();
    QVERIFY(mock->visible);
}

void NotificationTest::testClosing()
{
    Notification notification("Something", "Here");
    MockNotification *mock = *(notifications.begin());

    QSignalSpy closed(&notification, SIGNAL(closed()));
    mock->close();
    QCOMPARE(closed.count(), 1);
}

void NotificationTest::testActions()
{
    Notification notification("Something", "Here");
    MockNotification *mock = *(notifications.begin());

    QSignalSpy actionInvoked(&notification,
                             SIGNAL(actionInvoked(const QString &)));
    notification.addAction("SayHello", "Say Hello");
    notification.addAction("ok", "OK");
    notification.addAction("cancel", "Cancel");

    QCOMPARE(actionInvoked.count(), 0);

    mock->invokeAction("ok");
    QCOMPARE(actionInvoked.count(), 1);
    QCOMPARE(actionInvoked.at(0).at(0).toString(), QString("ok"));
    actionInvoked.clear();

    mock->invokeAction("cancel");
    QCOMPARE(actionInvoked.count(), 1);
    QCOMPARE(actionInvoked.at(0).at(0).toString(), QString("cancel"));
    actionInvoked.clear();

    mock->invokeAction("SayHello");
    QCOMPARE(actionInvoked.count(), 1);
    QCOMPARE(actionInvoked.at(0).at(0).toString(), QString("SayHello"));
}

QTEST_MAIN(NotificationTest);

#include "tst_notification.moc"

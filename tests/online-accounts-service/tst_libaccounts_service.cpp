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

#include "debug.h"
#include "libaccounts-service.h"

#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>
#include <QDBusConnection>
#include <QDebug>
#include <QProcess>
#include <QSignalSpy>
#include <QString>
#include <QTest>

using namespace OnlineAccountsUi;

#define TEST_SERVICE_NAME \
    "com.ubuntu.OnlineAccountsUi.LibaccountsService.Test"
#define TEST_OBJECT_PATH "/"

class LibaccountsServiceTest: public QObject
{
    Q_OBJECT

public:
    LibaccountsServiceTest();

private:
    QProcess *requestStore(const QString &args, bool showError = false) {
        QString command = QStringLiteral("gdbus call --session "
                                         "--dest " TEST_SERVICE_NAME " "
                                         "--object-path " TEST_OBJECT_PATH " "
                                         "--method com.google.code.AccountsSSO.Accounts.Manager.store ");
        QProcess *process = new QProcess(this);
        if (showError) {
            process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
        }
        process->start(command + args);
        process->waitForStarted();
        return process;
    }

private Q_SLOTS:
    void init();
    void testProfile_data();
    void testProfile();
    void testFailure();
    void testAccount_data();
    void testAccount();
    void testSettings_data();
    void testSettings();

private:
    LibaccountsService m_service;
};

/* Mocking libaccounts-qt { */
class ManagerController {
public:
    ManagerController(): lastLoadedAccount(0) { m_instance = this; }
    ~ManagerController() { m_instance = 0; }
    static ManagerController *instance() { return m_instance; }

    void setServices(const QStringList &services) { m_services = services; }

public:
    Accounts::Account *lastLoadedAccount;

private:
    friend class Accounts::Manager;
    QStringList m_services;
    static ManagerController *m_instance;
};

ManagerController *ManagerController::m_instance = 0;

typedef QHash<QString,QVariantMap> ServiceSettings;
typedef QHash<QString,QSet<QString> > RemovedKeys;

class AccountController: public QObject {
    Q_OBJECT
public:
    AccountController(Accounts::Account *account):
        QObject(account),
        m_id(0),
        m_wasDeleted(false),
        m_syncWasCalled(false),
        m_account(account) {
        m_controllers[account] = this;
    }

    static AccountController *mock(Accounts::Account *account) {
        return m_controllers[account];
    }

    bool syncWasCalled() const { return m_syncWasCalled; }
    void doSync(Accounts::Error error = Accounts::Error()) {
        if (error.type() == Accounts::Error::NoError) {
            QMetaObject::invokeMethod(m_account, "synced", Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(m_account, "error", Qt::QueuedConnection,
                                      Q_ARG(Accounts::Error, error));
        }
    }

protected:
    void syncCalled() { m_syncWasCalled = true; }

public:
    quint32 m_id;
    QString m_provider;
    ServiceSettings m_serviceSettings;
    RemovedKeys m_removedKeys;
    bool m_wasDeleted;
    bool m_syncWasCalled;
private:
    friend class Accounts::Account;
    static QHash<Accounts::Account *,AccountController*> m_controllers;
    Accounts::Account *m_account;
};

QHash<Accounts::Account *,AccountController*> AccountController::m_controllers;

struct _AgService {
    _AgService(const QString &name): name(name) {}
    QString name;
};

namespace Accounts {

Service::Service():
    m_service(0),
    m_tags(0)
{
}

Service::Service(AgService *service, ReferenceMode):
    m_service(service),
    m_tags(0)
{
}

Service::Service(const Service &other):
    m_service(other.m_service ? new _AgService(other.m_service->name) : 0),
    m_tags(0)
{
}

Service &Service::operator=(const Service &other)
{
    delete m_service;
    m_service = other.m_service ? new _AgService(other.m_service->name) : 0;
    return *this;
}

Service::~Service()
{
    delete m_service;
}

bool Service::isValid() const
{
    return m_service != 0;
}

class Account::Private {
public:
    Private() {}

    QString m_selectedService;
    AccountController *m_controller;
};

Account::Account(Private *d, QObject *parent):
    QObject(parent),
    d(d)
{
    d->m_controller = new AccountController(this);
}

Account::~Account()
{
    delete d;
}

AccountId Account::id() const
{
    return d->m_controller->m_id;
}

void Account::selectService(const Service &service)
{
    d->m_selectedService = service.m_service ?
        service.m_service->name : QString();
}

void Account::setValue(const QString &key, const QVariant &value)
{
    QVariantMap &settings =
        d->m_controller->m_serviceSettings[d->m_selectedService];
    settings.insert(key, value);
}

void Account::remove(const QString &key)
{
    QSet<QString> &removedKeys =
        d->m_controller->m_removedKeys[d->m_selectedService];
    removedKeys.insert(key);
}

void Account::sync()
{
    d->m_controller->syncCalled();
}

void Account::remove()
{
    d->m_controller->m_wasDeleted = true;
}

Watch::~Watch()
{
}

class Manager::Private {
public:
    Private() {}

    ManagerController m_controller;
};

Manager::Manager(QObject *parent):
    QObject(parent),
    d(new Private())
{
}

Manager::~Manager()
{
    delete d;
}

Account *Manager::account(const AccountId &id) const
{
    Account::Private *accountD = new Account::Private();
    d->m_controller.lastLoadedAccount =
        new Account(accountD, const_cast<Manager*>(this));
    accountD->m_controller->m_id = id;
    return d->m_controller.lastLoadedAccount;
}

Account *Manager::createAccount(const QString &providerName)
{
    Account::Private *accountD = new Account::Private();
    d->m_controller.lastLoadedAccount = new Account(accountD, this);
    accountD->m_controller->m_provider = providerName;
    return d->m_controller.lastLoadedAccount;
}

Service Manager::service(const QString &serviceName) const
{
    if (d->m_controller.m_services.contains(serviceName)) {
        return Service(new _AgService(serviceName));
    } else {
        return Service();
    }
}

} // namespace

/* } mocking libaccounts-qt */

/* mocking utils.cpp { */
namespace OnlineAccountsUi {

static QString staticApparmorProfile;

QString apparmorProfileOfPeer(const QDBusMessage &)
{
    return staticApparmorProfile;
}

void setApparmorProfile(const QString &profile)
{
    staticApparmorProfile = profile;
}

} // namespace
/* } mocking utils.cpp */

Q_DECLARE_METATYPE(QProcess::ExitStatus)

LibaccountsServiceTest::LibaccountsServiceTest():
    QObject(0)
{
    QDBusConnection conn = QDBusConnection::sessionBus();
    conn.registerService(TEST_SERVICE_NAME);
    conn.registerObject(TEST_OBJECT_PATH, &m_service,
                        QDBusConnection::ExportAllContents);

    qRegisterMetaType<QProcess::ExitStatus>();
    setLoggingLevel(2);
}

void LibaccountsServiceTest::init()
{
    ManagerController *mc = ManagerController::instance();
    mc->lastLoadedAccount = 0;
}

void LibaccountsServiceTest::testProfile_data()
{
    QTest::addColumn<QString>("profile");
    QTest::addColumn<QString>("provider");
    QTest::addColumn<bool>("mustPass");

    QTest::newRow("non click, mismatch") <<
        "appProfile" << "OneProvider" << false;

    QTest::newRow("non click, match") <<
        "theProfile" << "theProfile" << true;

    QTest::newRow("click, mismatch") <<
        "com.ubuntu.package_app_0.1" << "com.ubuntu.package_other" << false;

    QTest::newRow("click, match") <<
        "com.ubuntu.package_app_0.2" << "com.ubuntu.package_app" << true;
}

void LibaccountsServiceTest::testProfile()
{
    QFETCH(QString, profile);
    QFETCH(QString, provider);
    QFETCH(bool, mustPass);

    setApparmorProfile(profile);
    QString params = QString::fromUtf8("0 true false %1 []").arg(provider);
    QProcess *client = requestStore(params);
    QSignalSpy finished(client, SIGNAL(finished(int,QProcess::ExitStatus)));

    if (mustPass) {
        ManagerController *mc = ManagerController::instance();
        QTRY_VERIFY(mc->lastLoadedAccount != 0);
        AccountController *ac = AccountController::mock(mc->lastLoadedAccount);
        QTRY_COMPARE(ac->syncWasCalled(), true);
        ac->doSync();
    }

    finished.wait();

    QByteArray stdErr = client->readAllStandardError();
    QCOMPARE(stdErr.contains("Profile/provider mismatch"), !mustPass);
}

void LibaccountsServiceTest::testFailure()
{
    setApparmorProfile("MyProvider");
    QProcess *client = requestStore("0 true false MyProvider []");
    QSignalSpy finished(client, SIGNAL(finished(int,QProcess::ExitStatus)));

    ManagerController *mc = ManagerController::instance();
    QTRY_VERIFY(mc->lastLoadedAccount != 0);
    AccountController *ac = AccountController::mock(mc->lastLoadedAccount);
    QTRY_COMPARE(ac->syncWasCalled(), true);

    // return an error
    Accounts::Error error(Accounts::Error::Database, "hi there");
    ac->doSync(error);
    finished.wait();

    QVERIFY(client->readAllStandardError().contains("hi there"));
}

void LibaccountsServiceTest::testAccount_data()
{
    QTest::addColumn<QString>("clientSettings");
    QTest::addColumn<QString>("provider");
    QTest::addColumn<quint32>("accountId");
    QTest::addColumn<bool>("deleted");

    QTest::newRow("new account") <<
        "0 true false Cool" <<
        "Cool" << quint32(0) << false;

    QTest::newRow("existing account") <<
        "5 false false Bad" <<
        "Bad" << quint32(5) << false;

    QTest::newRow("deleting account") <<
        "7 false true Die" <<
        "Die" << quint32(7) << true;
}

void LibaccountsServiceTest::testAccount()
{
    QFETCH(QString, clientSettings);
    QFETCH(QString, provider);
    QFETCH(quint32, accountId);
    QFETCH(bool, deleted);

    setApparmorProfile(provider);

    ManagerController *mc = ManagerController::instance();

    QString params = QString::fromUtf8("%1 []").arg(clientSettings);
    QProcess *client = requestStore(params, true);
    QSignalSpy finished(client, SIGNAL(finished(int,QProcess::ExitStatus)));

    QTRY_VERIFY(mc->lastLoadedAccount != 0);
    AccountController *ac = AccountController::mock(mc->lastLoadedAccount);
    QTRY_COMPARE(ac->syncWasCalled(), true);

    QCOMPARE(ac->m_id, accountId);
    if (accountId == 0) {
        QCOMPARE(ac->m_provider, provider);
    }
    QCOMPARE(ac->m_wasDeleted, deleted);

    ac->doSync();

    finished.wait();
}

void LibaccountsServiceTest::testSettings_data()
{
    QTest::addColumn<QString>("clientSettings");
    QTest::addColumn<ServiceSettings>("settings");
    QTest::addColumn<RemovedKeys>("removedKeys");

    QHash<QString,QVariantMap> settings;
    QHash<QString,QSet<QString> > removedKeys;

    QTest::newRow("no services") <<
        "\"[]\"" <<
        settings << removedKeys;
    QTest::newRow("no settings") <<
        "\"[('cool', 'type', 3, {}, [])]\"" <<
        settings << removedKeys;

    settings["cool"].insert("enabled", false);
    settings["cool"].insert("name", QString("Bob"));
    QTest::newRow("some keys changed") <<
        "\"[('cool', 'type', 3, {'enabled': <false>, 'name': <'Bob'>}, [])]\"" <<
        settings << removedKeys;
    settings.clear();

    removedKeys["cool"].insert("enabled");
    removedKeys["cool"].insert("id");
    QTest::newRow("some keys removed") <<
        "\"[('cool', 'type', 3, {}, ['enabled','id'])]\"" <<
        settings << removedKeys;
    removedKeys.clear();

    settings["cool"].insert("enabled", true);
    settings["cool"].insert("name", QString("Tom"));
    removedKeys["cool"].insert("id");
    settings["bad"].insert("port", quint32(4000));
    removedKeys["bad"].insert("name");
    QTest::newRow("two services, lots of changes") <<
        "\"["
        "('cool', 'type', 3, {'enabled': <true>, 'name': <'Tom'>}, ['id']),"
        "('bad', 'btype', 2, {'port': <uint32 4000>}, ['name'])"
        "]\"" <<
        settings << removedKeys;
    settings.clear();
    removedKeys.clear();

    QTest::newRow("invalid service") <<
        "\"[('findme', 'type', 3, {}, ['enabled','id'])]\"" <<
        settings << removedKeys;
}

void LibaccountsServiceTest::testSettings()
{
    QFETCH(QString, clientSettings);
    QFETCH(ServiceSettings, settings);
    QFETCH(RemovedKeys, removedKeys);

    setApparmorProfile("MyProvider");

    ManagerController *mc = ManagerController::instance();
    mc->setServices(QStringList() << "cool" << "bad");

    QString params =
        QString::fromUtf8("0 true false MyProvider %1").arg(clientSettings);
    QProcess *client = requestStore(params, true);
    QSignalSpy finished(client, SIGNAL(finished(int,QProcess::ExitStatus)));

    QTRY_VERIFY(mc->lastLoadedAccount != 0);
    AccountController *ac = AccountController::mock(mc->lastLoadedAccount);
    QTRY_COMPARE(ac->syncWasCalled(), true);

    QCOMPARE(ac->m_serviceSettings, settings);
    QCOMPARE(ac->m_removedKeys, removedKeys);

    ac->doSync();

    finished.wait();
}

QTEST_MAIN(LibaccountsServiceTest);

#include "tst_libaccounts_service.moc"

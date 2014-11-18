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

#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>
#include <QDebug>
#include <QDir>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTest>

using namespace OnlineAccountsUi;

class AccessModelTest: public QObject
{
    Q_OBJECT

public:
    AccessModelTest();

private Q_SLOTS:
    void initTestCase();
    void testEmpty();
    void testProxy();
    void testEnabling();

private:
    void clearDb();
};

AccessModelTest::AccessModelTest():
    QObject(0)
{
    setLoggingLevel(2);
}

void AccessModelTest::clearDb()
{
    QDir dbroot(QString::fromLatin1(qgetenv("ACCOUNTS")));
    dbroot.remove("accounts.db");
}

void AccessModelTest::initTestCase()
{
    qputenv("ACCOUNTS", "/tmp/");
    qputenv("AG_APPLICATIONS", TEST_DATA_DIR);
    qputenv("AG_SERVICES", TEST_DATA_DIR);
    qputenv("AG_SERVICE_TYPES", TEST_DATA_DIR);
    qputenv("AG_PROVIDERS", TEST_DATA_DIR);
    qputenv("XDG_DATA_HOME", TEST_DATA_DIR);

    clearDb();
}

void AccessModelTest::testEmpty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import Ubuntu.OnlineAccounts 0.1\n"
                      "AccountServiceModel {\n"
                      "  provider: \"cool\"\n"
                      "  service: \"global\"\n"
                      "}",
                      QUrl());
    QAbstractListModel *accountModel =
        qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(accountModel != 0);

    QCOMPARE(accountModel->property("count").toInt(), 0);

    /* Create the access model */
    AccessModel *model = new AccessModel(this);
    QVERIFY(model != 0);

    QSignalSpy countChanged(model, SIGNAL(countChanged()));
    QSignalSpy rowsInserted(model, SIGNAL(rowsInserted(const QModelIndex&,int,int)));
    QSignalSpy rowsRemoved(model, SIGNAL(rowsRemoved(const QModelIndex&,int,int)));

    QCOMPARE(model->applicationId(), QString());
    QVERIFY(model->accountModel() == 0);
    QCOMPARE(model->rowCount(), 0);

    model->setAccountModel(accountModel);
    QCOMPARE(model->rowCount(), 0);

    QCOMPARE(countChanged.count(), 0);
    QCOMPARE(rowsInserted.count(), 0);
    QCOMPARE(rowsRemoved.count(), 0);

    delete model;
    delete accountModel;
}

void AccessModelTest::testProxy()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import Ubuntu.OnlineAccounts 0.1\n"
                      "AccountServiceModel {\n"
                      "  provider: \"cool\"\n"
                      "  service: \"global\"\n"
                      "}",
                      QUrl());
    QAbstractListModel *accountModel =
        qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(accountModel != 0);

    QCOMPARE(accountModel->property("count").toInt(), 0);

    /* Create the access model */
    AccessModel *model = new AccessModel(this);
    QVERIFY(model != 0);

    model->setAccountModel(accountModel);
    model->setApplicationId("mailer");
    QCOMPARE(model->rowCount(), 0);

    /* Now add and remove accounts, and see that the accessModel picks them up */
    QSignalSpy rowsInserted(model,
                            SIGNAL(rowsInserted(const QModelIndex&,int,int)));
    QSignalSpy rowsRemoved(model,
                           SIGNAL(rowsRemoved(const QModelIndex&,int,int)));

    Accounts::Manager *manager = new Accounts::Manager(this);
    Accounts::Account *account1 = manager->createAccount("cool");
    QVERIFY(account1 != 0);
    account1->setEnabled(true);
    account1->setDisplayName("CoolAccount");
    account1->syncAndBlock();

    rowsInserted.wait();
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(rowsInserted.count(), 1);
    QCOMPARE(rowsRemoved.count(), 0);
    rowsInserted.clear();
    QCOMPARE(model->get(0, "displayName").toString(), QString("CoolAccount"));

    /* Create a second account */
    Accounts::Account *account2 = manager->createAccount("cool");
    QVERIFY(account2 != 0);
    account2->setEnabled(true);
    account2->setDisplayName("UncoolAccount");
    account2->syncAndBlock();

    rowsInserted.wait();
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(rowsInserted.count(), 1);
    int row = rowsInserted.at(0).at(1).toInt();
    QCOMPARE(rowsRemoved.count(), 0);
    rowsInserted.clear();
    QCOMPARE(model->get(row, "displayName").toString(), QString("UncoolAccount"));

    /* Delete the first account */
    account1->remove();
    account1->syncAndBlock();
    rowsRemoved.wait();
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(rowsInserted.count(), 0);
    QCOMPARE(rowsRemoved.count(), 1);
    rowsRemoved.clear();
    QCOMPARE(model->get(0, "displayName").toString(), QString("UncoolAccount"));

    /* Delete also the second account */
    account2->remove();
    account2->syncAndBlock();
    rowsRemoved.wait();
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(rowsInserted.count(), 0);
    QCOMPARE(rowsRemoved.count(), 1);
    rowsRemoved.clear();

    delete model;
    delete accountModel;
}

void AccessModelTest::testEnabling()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import Ubuntu.OnlineAccounts 0.1\n"
                      "AccountServiceModel {\n"
                      "  provider: \"cool\"\n"
                      "  service: \"global\"\n"
                      "}",
                      QUrl());
    QAbstractListModel *accountModel =
        qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(accountModel != 0);

    QCOMPARE(accountModel->property("count").toInt(), 0);

    /* Create the access model */
    AccessModel *model = new AccessModel(this);
    QVERIFY(model != 0);

    model->setAccountModel(accountModel);
    model->setApplicationId("mailer");
    QCOMPARE(model->rowCount(), 0);

    /* Now add two accounts, but verify that the model should expose only the
     * one which has at least one service disabled */
    QSignalSpy rowsInserted(model,
                            SIGNAL(rowsInserted(const QModelIndex&,int,int)));
    QSignalSpy rowsRemoved(model,
                           SIGNAL(rowsRemoved(const QModelIndex&,int,int)));

    Accounts::Manager *manager = new Accounts::Manager(this);
    Accounts::Service coolMail = manager->service("coolmail");
    Accounts::Service coolShare = manager->service("coolshare");
    Accounts::Account *account1 = manager->createAccount("cool");
    QVERIFY(account1 != 0);
    account1->setEnabled(true);
    account1->setDisplayName("CoolAccount");
    account1->selectService(coolMail);
    account1->setEnabled(true);
    account1->syncAndBlock();

    rowsInserted.wait();
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(rowsInserted.count(), 1);
    QCOMPARE(rowsRemoved.count(), 0);
    rowsInserted.clear();
    QCOMPARE(model->get(0, "displayName").toString(), QString("CoolAccount"));

    /* Create a second account, enable all of its services */
    Accounts::Account *account2 = manager->createAccount("cool");
    QVERIFY(account2 != 0);
    account2->setEnabled(true);
    account2->setDisplayName("UncoolAccount");
    account2->selectService(coolMail);
    account2->setEnabled(true);
    account2->selectService(coolShare);
    account2->setEnabled(true);
    account2->syncAndBlock();

    /* Verify that a row is *not* added */
    QTest::qWait(50);
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(rowsInserted.count(), 0);
    QCOMPARE(rowsRemoved.count(), 0);
    rowsInserted.clear();

    delete model;
    delete accountModel;
}

QTEST_MAIN(AccessModelTest);

#include "tst_access_model.moc"

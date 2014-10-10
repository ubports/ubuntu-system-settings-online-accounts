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

#include <Accounts/Account>
#include <Accounts/Manager>
#include <QDebug>
#include <QDir>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTest>

#define TEST_DIR "/tmp/tst_application_manager"

using namespace OnlineAccountsUi;

class ApplicationManagerTest: public QObject
{
    Q_OBJECT

public:
    ApplicationManagerTest();

private Q_SLOTS:
    void initTestCase();
    void testNonExistingApp();
    void testApplicationInfo_data();
    void testApplicationInfo();
    void testAclAdd_data();
    void testAclAdd();
    void testAclRemove_data();
    void testAclRemove();
    void testApplicationFromProfile_data();
    void testApplicationFromProfile();
    void testProviderInfo_data();
    void testProviderInfo();

private:
    void clearApplicationsDir();
    void clearProvidersDir();
    void clearTestDir();
    void writeAccountsFile(const QString &name, const QString &contents);

private:
    QDir m_testDir;
    QDir m_accountsDir;
};

ApplicationManagerTest::ApplicationManagerTest():
    QObject(0),
    m_testDir(TEST_DIR),
    m_accountsDir(TEST_DIR "/accounts")
{
}

void ApplicationManagerTest::clearApplicationsDir()
{
    QDir applicationsDir = m_accountsDir;
    if (applicationsDir.cd("applications")) {
        applicationsDir.removeRecursively();
        applicationsDir.mkpath(".");
    }
}

void ApplicationManagerTest::clearProvidersDir()
{
    QDir providersDir = m_accountsDir;
    if (providersDir.cd("providers")) {
        providersDir.removeRecursively();
        providersDir.mkpath(".");
    }
}

void ApplicationManagerTest::clearTestDir()
{
    m_testDir.removeRecursively();
    m_testDir.mkpath(".");
}

void ApplicationManagerTest::writeAccountsFile(const QString &name,
                                               const QString &contents)
{
    /* Create files into the appropriate directory:
     * .provider files go in "providers", .application in "applications" and so
     * on: so we just need to take the file extension and add one "s".
     */
    QFileInfo fileInfo(name);
    QString subDirName = fileInfo.suffix() + "s";
    m_accountsDir.mkpath(subDirName);
    QDir subDir = m_accountsDir;
    subDir.cd(subDirName);

    QFile file(subDir.filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write file" << name;
        return;
    }

    file.write(contents.toUtf8());
}

void ApplicationManagerTest::initTestCase()
{
    qputenv("ACCOUNTS", TEST_DIR);
    qputenv("XDG_DATA_HOME", TEST_DIR);

    clearTestDir();

    /* Create a few service files */
    writeAccountsFile("cool-mail.service",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<service id=\"cool-mail\">\n"
        "  <type>tstemail</type>\n"
        "  <name>Cool Mail</name>\n"
        "  <provider>cool</provider>\n"
        "</service>");
    writeAccountsFile("cool-sharing.service",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<service id=\"cool-sharing\">\n"
        "  <type>tstsharing</type>\n"
        "  <name>Cool Sharing</name>\n"
        "  <provider>cool</provider>\n"
        "</service>");
    writeAccountsFile("bad-mail.service",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<service id=\"bad-mail\">\n"
        "  <type>tstemail</type>\n"
        "  <name>Bad Mail</name>\n"
        "  <provider>bad</provider>\n"
        "</service>");
    writeAccountsFile("bad-sharing.service",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<service id=\"bad-sharing\">\n"
        "  <type>tstsharing</type>\n"
        "  <name>Bad Sharing</name>\n"
        "  <provider>bad</provider>\n"
        "</service>");

    /* And the relative providers */
    writeAccountsFile("cool.provider",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider id=\"cool\">\n"
        "  <name>Cool provider</name>\n"
        "</provider>");
    writeAccountsFile("bad.provider",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider id=\"bad\">\n"
        "  <name>Bad provider</name>\n"
        "</provider>");
}

void ApplicationManagerTest::testNonExistingApp()
{
    ApplicationManager manager;

    QVariantMap info = manager.applicationInfo("com.ubuntu.test_MyApp",
                                               "com.ubuntu.test_MyApp_0.1");
    QVERIFY(info.isEmpty());

    QStringList acl;
    acl << "one" << "two";
    QStringList newAcl = manager.addApplicationToAcl(acl, "com.ubuntu.test_MyApp");
    QCOMPARE(newAcl, acl);

    newAcl = manager.removeApplicationFromAcl(acl, "com.ubuntu.test_MyApp");
    QCOMPARE(newAcl, acl);
}

void ApplicationManagerTest::testApplicationInfo_data()
{
    QTest::addColumn<QString>("applicationId");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QString>("profile");
    QTest::addColumn<QStringList>("services");

    QTest::newRow("no-services") <<
        "com.ubuntu.test_MyApp" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_MyApp\">\n"
        "  <description>My application</description>\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "</application>" <<
        "com.ubuntu.test_MyApp_0.2" <<
        QStringList();

    QTest::newRow("no-valid-services") <<
        "com.ubuntu.test_MyApp2" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_MyApp2\">\n"
        "  <description>My application 2</description>\n"
        "  <service-types>\n"
        "    <service-type id=\"some type\">Do something</service-type>\n"
        "  </service-types>\n"
        "  <profile>com.ubuntu.test_MyApp2_0.2</profile>\n"
        "</application>" <<
        "com.ubuntu.test_MyApp2_0.2" <<
        QStringList();

    QTest::newRow("email-services") <<
        "com.ubuntu.test_MyApp3" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_MyApp3\">\n"
        "  <description>My application 3</description>\n"
        "  <service-types>\n"
        "    <service-type id=\"tstemail\">\n"
        "      <description>Send email</description>\n"
        "    </service-type>\n"
        "  </service-types>\n"
        "  <profile>com.ubuntu.test_MyApp3_0.2</profile>\n"
        "</application>" <<
        "com.ubuntu.test_MyApp3_0.2" <<
        (QStringList() << "cool-mail" << "bad-mail");

    QTest::newRow("cool-services") <<
        "com.ubuntu.test_MyApp4" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_MyApp4\">\n"
        "  <description>My application 4</description>\n"
        "  <services>\n"
        "    <service id=\"cool-mail\">\n"
        "      <description>Send email</description>\n"
        "    </service>\n"
        "    <service id=\"cool-sharing\">\n"
        "      <description>Share stuff</description>\n"
        "    </service>\n"
        "  </services>\n"
        "  <profile>com.ubuntu.test_MyApp4_0.2</profile>\n"
        "</application>" <<
        "com.ubuntu.test_MyApp4_0.2" <<
        (QStringList() << "cool-mail" << "cool-sharing");
}

void ApplicationManagerTest::testApplicationInfo()
{
    clearApplicationsDir();

    QFETCH(QString, applicationId);
    QFETCH(QString, contents);
    QFETCH(QString, profile);
    QFETCH(QStringList, services);

    writeAccountsFile(applicationId + ".application", contents);

    ApplicationManager manager;

    QVariantMap info = manager.applicationInfo(applicationId, profile);
    QCOMPARE(info.value("id").toString(), applicationId);
    QCOMPARE(info.value("profile").toString(), profile);
    QCOMPARE(info.value("services").toStringList().toSet(), services.toSet());
}

void ApplicationManagerTest::testAclAdd_data()
{
    QTest::addColumn<QString>("applicationId");
    QTest::addColumn<QStringList>("oldAcl");
    QTest::addColumn<QStringList>("newAcl");

    QTest::newRow("add to empty") <<
        "com.ubuntu.test_One" <<
        QStringList() <<
        (QStringList() << "com.ubuntu.test_One_0.1");

    QTest::newRow("add to one") <<
        "com.ubuntu.test_One" <<
        (QStringList() << "some app") <<
        (QStringList() << "some app" << "com.ubuntu.test_One_0.1");

    QTest::newRow("add existing") <<
        "com.ubuntu.test_One" <<
        (QStringList() << "com.ubuntu.test_One_0.1" << "some app") <<
        (QStringList() << "com.ubuntu.test_One_0.1" << "some app");
}

void ApplicationManagerTest::testAclAdd()
{
    clearApplicationsDir();

    writeAccountsFile("com.ubuntu.test_One.application",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_One\">\n"
        "  <description>My application</description>\n"
        "  <profile>com.ubuntu.test_One_0.1</profile>\n"
        "</application>");
    writeAccountsFile("com.ubuntu.test_Two.application",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_Two\">\n"
        "  <description>My application</description>\n"
        "  <profile>com.ubuntu.test_Two_0.2</profile>\n"
        "</application>");

    QFETCH(QString, applicationId);
    QFETCH(QStringList, oldAcl);
    QFETCH(QStringList, newAcl);

    ApplicationManager manager;

    QStringList acl = manager.addApplicationToAcl(oldAcl, applicationId);
    QCOMPARE(acl.toSet(), newAcl.toSet());
}

void ApplicationManagerTest::testAclRemove_data()
{
    QTest::addColumn<QString>("applicationId");
    QTest::addColumn<QStringList>("oldAcl");
    QTest::addColumn<QStringList>("newAcl");

    QTest::newRow("remove from empty") <<
        "com.ubuntu.test_One" <<
        QStringList() <<
        QStringList();

    QTest::newRow("remove from one") <<
        "com.ubuntu.test_One" <<
        (QStringList() << "com.ubuntu.test_One_0.1") <<
        QStringList();

    QTest::newRow("remove from two") <<
        "com.ubuntu.test_One" <<
        (QStringList() << "com.ubuntu.test_One_0.1" << "some app") <<
        (QStringList() << "some app");

    QTest::newRow("remove from missing") <<
        "com.ubuntu.test_One" <<
        (QStringList() << "some other app" << "some app") <<
        (QStringList() << "some other app" << "some app");

    QTest::newRow("remove multiple versions") <<
        "com.ubuntu.test_One" <<
        (QStringList() << "some other app" << "com.ubuntu.test_One_0.1" <<
         "com.ubuntu.test_One_0.3" << "some app" <<
         "com.ubuntu.test_One_0.2") <<
        (QStringList() << "some other app" << "some app");
}

void ApplicationManagerTest::testAclRemove()
{
    clearApplicationsDir();

    writeAccountsFile("com.ubuntu.test_One.application",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_One\">\n"
        "  <description>My application</description>\n"
        "  <profile>com.ubuntu.test_One_0.1</profile>\n"
        "</application>");
    writeAccountsFile("com.ubuntu.test_Two.application",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_Two\">\n"
        "  <description>My application</description>\n"
        "  <profile>com.ubuntu.test_Two_0.2</profile>\n"
        "</application>");

    QFETCH(QString, applicationId);
    QFETCH(QStringList, oldAcl);
    QFETCH(QStringList, newAcl);

    ApplicationManager manager;

    QStringList acl = manager.removeApplicationFromAcl(oldAcl, applicationId);
    QCOMPARE(acl.toSet(), newAcl.toSet());
}

void ApplicationManagerTest::testApplicationFromProfile_data()
{
    QTest::addColumn<QString>("applicationId");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QString>("profile");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("id-same-as-profile") <<
        "com.ubuntu.test_MyApp_0.2" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_MyApp\">\n"
        "  <description>My application</description>\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "</application>" <<
        "com.ubuntu.test_MyApp_0.2" <<
        true;

    QTest::newRow("id-with-no-version") <<
        "com.ubuntu.test_MyApp2" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_MyApp2\">\n"
        "  <description>My application 2</description>\n"
        "  <profile>com.ubuntu.test_MyApp2_0.2</profile>\n"
        "</application>" <<
        "com.ubuntu.test_MyApp2_0.2" <<
        true;

    QTest::newRow("id-is-just-package") <<
        "com.ubuntu.test" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_MyApp3\">\n"
        "  <description>My application 3</description>\n"
        "  <profile>com.ubuntu.test_MyApp3_0.2</profile>\n"
        "</application>" <<
        "com.ubuntu.test_MyApp3_0.2" <<
        true;

    QTest::newRow("not found") <<
        "com.ubuntu.test_MyApp5" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"com.ubuntu.test_MyApp4\">\n"
        "  <description>My application 4</description>\n"
        "  <profile>com.ubuntu.test_MyApp4_0.2</profile>\n"
        "</application>" <<
        "com.ubuntu.test_MyApp4_0.2" <<
        false;
}

void ApplicationManagerTest::testApplicationFromProfile()
{
    clearApplicationsDir();

    QFETCH(QString, applicationId);
    QFETCH(QString, contents);
    QFETCH(QString, profile);
    QFETCH(bool, isValid);

    writeAccountsFile(applicationId + ".application", contents);

    ApplicationManager manager;

    Accounts::Application app = manager.applicationFromProfile(profile);
    if (!isValid) {
        QVERIFY(!app.isValid());
    } else {
        QVERIFY(app.isValid());
        QCOMPARE(app.name(), applicationId);
    }
}

void ApplicationManagerTest::testProviderInfo_data()
{
    QTest::addColumn<QString>("providerId");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QString>("profile");
    QTest::addColumn<QString>("packageDir");
    QTest::addColumn<bool>("isSingleAccount");

    QTest::newRow("no profile") <<
        "com.ubuntu.test_MyPlugin" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider id=\"com.ubuntu.test_MyPlugin\">\n"
        "  <name>My Plugin</name>\n"
        "</provider>" <<
        QString() <<
        QString() <<
        false;

    QTest::newRow("single account") <<
        "com.ubuntu.test_MyPlugin" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider id=\"com.ubuntu.test_MyPlugin\">\n"
        "  <name>My Plugin</name>\n"
        "  <single-account>true</single-account>\n"
        "</provider>" <<
        QString() <<
        QString() <<
        true;

    QTest::newRow("no package-dir") <<
        "com.ubuntu.test_MyPlugin2" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider id=\"com.ubuntu.test_MyPlugin2\">\n"
        "  <name>My Plugin</name>\n"
        "  <profile>com.ubuntu.test_MyPlugin2_0.2</profile>\n"
        "  <single-account>false</single-account>\n"
        "</provider>" <<
        "com.ubuntu.test_MyPlugin2_0.2" <<
        QString() <<
        false;

    QTest::newRow("with package-dir") <<
        "com.ubuntu.test_MyPlugin3" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider id=\"com.ubuntu.test_MyPlugin3\">\n"
        "  <name>My Plugin</name>\n"
        "  <profile>com.ubuntu.test_MyPlugin3_0.2</profile>\n"
        "  <package-dir>/opt/click.ubuntu.com/something</package-dir>\n"
        "</provider>" <<
        "com.ubuntu.test_MyPlugin3_0.2" <<
        "/opt/click.ubuntu.com/something" <<
        false;
}

void ApplicationManagerTest::testProviderInfo()
{
    clearProvidersDir();

    QFETCH(QString, providerId);
    QFETCH(QString, contents);
    QFETCH(QString, profile);
    QFETCH(QString, packageDir);
    QFETCH(bool, isSingleAccount);

    writeAccountsFile(providerId + ".provider", contents);

    ApplicationManager manager;

    QVariantMap info = manager.providerInfo(providerId);
    QCOMPARE(info.value("id").toString(), providerId);
    QCOMPARE(info.value("profile").toString(), profile);
    QCOMPARE(info.value("package-dir").toString(), packageDir);
    QCOMPARE(info.value("isSingleAccount").toBool(), isSingleAccount);
}

QTEST_MAIN(ApplicationManagerTest);

#include "tst_application_manager.moc"

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

#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTest>

#define TEST_DIR "/tmp/hooks-test"

class OnlineAccountsHooksTest: public QObject
{
    Q_OBJECT

public:
    OnlineAccountsHooksTest();

private Q_SLOTS:
    void initTestCase();
    void testNoFiles();
    void testValidHooks_data();
    void testValidHooks();
    void testRemoval();
    void testUpdate();
    void testDesktopEntry_data();
    void testDesktopEntry();
    void testServiceType_data();
    void testServiceType();

private:
    void clearHooksDir();
    void clearInstallDir();
    bool runHookProcess();
    void writeHookFile(const QString &name, const QString &contents);
    void writeInstalledFile(const QString &name, const QString &contents);
    void writePackageFile(const QString &name,
                          const QString &contents = QString());

private:
    QDir m_testDir;
    QDir m_hooksDir;
    QDir m_installDir;
    QDir m_packageDir;
};

OnlineAccountsHooksTest::OnlineAccountsHooksTest():
    QObject(0),
    m_testDir(TEST_DIR),
    m_hooksDir(TEST_DIR "/online-accounts-hooks"),
    m_installDir(TEST_DIR "/accounts"),
    m_packageDir(TEST_DIR "/package")
{
}

void OnlineAccountsHooksTest::clearHooksDir()
{
    m_hooksDir.removeRecursively();
    m_hooksDir.mkpath(".");
}

void OnlineAccountsHooksTest::clearInstallDir()
{
    m_installDir.removeRecursively();
    m_installDir.mkpath(".");
}

bool OnlineAccountsHooksTest::runHookProcess()
{
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start(HOOK_PROCESS);
    if (!process.waitForFinished()) return false;

    return process.exitCode() == EXIT_SUCCESS;
}

void OnlineAccountsHooksTest::writeHookFile(const QString &name,
                                            const QString &contents)
{
    QFile file(m_hooksDir.filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write file" << name;
        return;
    }

    file.write(contents.toUtf8());
}

void OnlineAccountsHooksTest::writeInstalledFile(const QString &name,
                                                 const QString &contents)
{
    QFileInfo fileInfo(name);
    m_installDir.mkpath(fileInfo.path());
    QFile file(m_installDir.filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write file" << name;
        return;
    }

    file.write(contents.toUtf8());
}

void OnlineAccountsHooksTest::writePackageFile(const QString &name,
                                               const QString &contents)
{
    QFileInfo fileInfo(name);
    m_packageDir.mkpath(fileInfo.path());
    QFile file(m_packageDir.filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not write file" << name;
        return;
    }

    file.write(contents.toUtf8());
}

void OnlineAccountsHooksTest::initTestCase()
{
    qputenv("XDG_DATA_HOME", TEST_DIR);
    qputenv("OAH_CLICK_DIR", m_packageDir.path().toUtf8());
    // The hook must be able to run without a D-Bus session
    qunsetenv("DBUS_SESSION_BUS_ADDRESS");

    clearHooksDir();
    clearInstallDir();
}

void OnlineAccountsHooksTest::testNoFiles()
{
    clearHooksDir();
    clearInstallDir();

    QVERIFY(runHookProcess());
    QVERIFY(m_hooksDir.entryList(QDir::Files).isEmpty());
    QVERIFY(m_installDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty());

    /* Create an unsupported hook file */
    writeHookFile("file.unsupported",
                  "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
                  "<service></service>");
    QVERIFY(!m_hooksDir.entryList(QDir::Files).isEmpty());

    QVERIFY(runHookProcess());
    QVERIFY(m_installDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty());
}

void OnlineAccountsHooksTest::testValidHooks_data()
{
    QTest::addColumn<QString>("hookName");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QString>("installedName");
    QTest::addColumn<QString>("profile");
    QTest::addColumn<QString>("icon");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("service") <<
        "com.ubuntu.test_MyApp_0.1.service" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<service>\n"
        "  <type>some type</type>\n"
        "  <name>my app service</name>\n"
        "  <provider>example</provider>\n"
        "</service>" <<
        "services/com.ubuntu.test_MyApp.service" <<
        "com.ubuntu.test_MyApp_0.1" <<
        QString() <<
        true;

    QTest::newRow("application") <<
        "com.ubuntu.test_MyApp_0.2.application" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application>" <<
        "applications/com.ubuntu.test_MyApp.application" <<
        "com.ubuntu.test_MyApp_0.2" <<
        QString() <<
        true;

    QTest::newRow("provider") <<
        "com.ubuntu.test_Plugin_0.1.provider" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider>\n"
        "  <name>My provider</name>\n"
        "</provider>" <<
        "providers/com.ubuntu.test_Plugin.provider" <<
        "com.ubuntu.test_Plugin_0.1" <<
        QString() <<
        true;

    QTest::newRow("provider-with-icon") <<
        "com.ubuntu.test_Plugin2_0.1.provider" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider>\n"
        "  <name>My provider</name>\n"
        "  <icon>MyApp.svg</icon>\n"
        "</provider>" <<
        "providers/com.ubuntu.test_Plugin2.provider" <<
        "com.ubuntu.test_Plugin2_0.1" <<
        m_packageDir.filePath("MyApp.svg") <<
        true;

    QTest::newRow("provider-with-wrong-icon") <<
        "com.ubuntu.test_Plugin3_0.1.provider" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider>\n"
        "  <name>My provider</name>\n"
        "  <icon>NonExisting.svg</icon>\n"
        "</provider>" <<
        "providers/com.ubuntu.test_Plugin3.provider" <<
        "com.ubuntu.test_Plugin3_0.1" <<
        "NonExisting.svg" <<
        true;

    QTest::newRow("provider-with-icon-subdir") <<
        "com.ubuntu.test_Plugin4_0.1.provider" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<provider>\n"
        "  <name>My provider</name>\n"
        "  <icon>subdir/pict.png</icon>\n"
        "</provider>" <<
        "providers/com.ubuntu.test_Plugin4.provider" <<
        "com.ubuntu.test_Plugin4_0.1" <<
        m_packageDir.filePath("subdir/pict.png") <<
        true;

    QTest::newRow("invalid application") <<
        "com.ubuntu.test_Invalid_0.1.application" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application /* invalid XML!" <<
        "applications/com.ubuntu.test_Invalid.application" <<
        "com.ubuntu.test_Invalid_0.1" <<
        QString() <<
        false;

    QTest::newRow("application with wrong id") <<
        "com.ubuntu.test_MyAppId_0.1.application" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application id=\"wrongid\">\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application>" <<
        "applications/com.ubuntu.test_MyAppId.application" <<
        "com.ubuntu.test_MyAppId_0.1" <<
        QString() <<
        true;
}

void OnlineAccountsHooksTest::testValidHooks()
{
    QFETCH(QString, hookName);
    QFETCH(QString, contents);
    QFETCH(QString, installedName);
    QFETCH(QString, profile);
    QFETCH(QString, icon);
    QFETCH(bool, isValid);

    /* Create the needed files in the package directory */
    writePackageFile("MyApp.svg");
    writePackageFile("subdir/pict.png");

    writeHookFile(hookName, contents);
    QVERIFY(runHookProcess());

    // check that the file has been created, if the file was valid
    QFileInfo fileInfo(m_installDir.absoluteFilePath(installedName));
    QCOMPARE(fileInfo.exists(), isValid);

    if (!isValid) return;

    QFile file(fileInfo.absoluteFilePath());
    QVERIFY(file.open(QIODevice::ReadOnly));

    // check that's a valid XML file
    QDomDocument doc;
    QVERIFY(doc.setContent(&file));

    // Check that the "id" attribute matches the file name
    QDomElement root = doc.documentElement();
    QCOMPARE(root.attribute("id"), fileInfo.completeBaseName());

    /* Check that a "profile" element has been added and that matches the
     * expected profile. */
    QDomElement profileElement = root.firstChildElement("profile");
    QVERIFY(!profileElement.isNull());
    QCOMPARE(profileElement.text(), profile);

    /* Check that the icon is correct */
    QDomElement iconElement = root.firstChildElement("icon");
    QCOMPARE(iconElement.text(), icon);
}

void OnlineAccountsHooksTest::testRemoval()
{
    clearHooksDir();
    clearInstallDir();

    QString stillInstalled("applications/com.ubuntu.test_StillInstalled.application");
    writeInstalledFile(stillInstalled,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "  <profile>com-ubuntu.test_StillInstalled_2.0</profile>\n"
        "</application>");
    QVERIFY(m_installDir.exists(stillInstalled));

    QString myApp("applications/com.ubuntu.test_MyApp.application");
    writeInstalledFile(myApp,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "  <profile>com-ubuntu.test_MyApp_3.0</profile>\n"
        "</application>");
    QVERIFY(m_installDir.exists(myApp));

    QString noProfile("applications/com.ubuntu.test_NoProfile.application");
    writeInstalledFile(noProfile,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application>");
    QVERIFY(m_installDir.exists(noProfile));

    writeHookFile("com-ubuntu.test_StillInstalled_2.0.application",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application>");

    QVERIFY(runHookProcess());

    QVERIFY(m_installDir.exists(stillInstalled));
    QVERIFY(!m_installDir.exists(myApp));
    QVERIFY(m_installDir.exists(noProfile));
}

void OnlineAccountsHooksTest::testUpdate()
{
    clearHooksDir();
    clearInstallDir();

    writeHookFile("com-ubuntu.test_MyApp_1.0.application",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application>");

    QVERIFY(runHookProcess());

    QString installedName("applications/com-ubuntu.test_MyApp.application");

    // check that the file has been created, and with the correct profile
    QFile file(m_installDir.absoluteFilePath(installedName));
    QVERIFY(file.open(QIODevice::ReadOnly));

    // check that's a valid XML file
    QDomDocument doc;
    QVERIFY(doc.setContent(&file));
    QDomElement root = doc.documentElement();
    QCOMPARE(root.firstChildElement("profile").text(),
             QString("com-ubuntu.test_MyApp_1.0"));

    /* Now remove the hook file and write a newer version of it */
    clearHooksDir();
    writeHookFile("com-ubuntu.test_MyApp_1.1.application",
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application>");

    QVERIFY(runHookProcess());

    // check that the file has been updated with the correct profile
    file.close();
    file.setFileName(m_installDir.absoluteFilePath(installedName));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(doc.setContent(&file));
    root = doc.documentElement();
    QCOMPARE(root.firstChildElement("profile").text(),
             QString("com-ubuntu.test_MyApp_1.1"));
}

void OnlineAccountsHooksTest::testDesktopEntry_data()
{
    QTest::addColumn<QString>("hookName");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QString>("installedName");
    QTest::addColumn<QString>("expectedDesktopEntry");

    QTest::newRow("no-entry") <<
        "com.ubuntu.test_MyApp_0.2.application" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application>" <<
        "applications/com.ubuntu.test_MyApp.application" <<
        "com.ubuntu.test_MyApp_0.2";

    QTest::newRow("with-desktop-entry") <<
        "com.ubuntu.test_Desktop_0.1.application" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <desktop-entry>something here</desktop-entry>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "</application>" <<
        "applications/com.ubuntu.test_Desktop.application" <<
        "something here";
}

void OnlineAccountsHooksTest::testDesktopEntry()
{
    QFETCH(QString, hookName);
    QFETCH(QString, contents);
    QFETCH(QString, installedName);
    QFETCH(QString, expectedDesktopEntry);

    writeHookFile(hookName, contents);
    QVERIFY(runHookProcess());

    // check that the file has been created, if the file was valid
    QFileInfo fileInfo(m_installDir.absoluteFilePath(installedName));
    QVERIFY(fileInfo.exists());

    QFile file(fileInfo.absoluteFilePath());
    QVERIFY(file.open(QIODevice::ReadOnly));

    // check that's a valid XML file
    QDomDocument doc;
    QVERIFY(doc.setContent(&file));

    QDomElement root = doc.documentElement();

    /* Check that a "desktop-entry" element has been added and that matches the
     * expected value. */
    QDomElement desktopEntryElement = root.firstChildElement("desktop-entry");
    QCOMPARE(desktopEntryElement.text(), expectedDesktopEntry);
}

void OnlineAccountsHooksTest::testServiceType_data()
{
    QTest::addColumn<QString>("hookName");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QString>("installedName");
    QTest::addColumn<QString>("expectedServiceType");

    QTest::newRow("no type") <<
        "com.ubuntu.test_MyApp_0.2.service" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<service>\n"
        "  <provider>google</provider>\n"
        "</service>" <<
        "services/com.ubuntu.test_MyApp.service" <<
        "com.ubuntu.test_MyApp";

    QTest::newRow("with service type") <<
        "com.ubuntu.test_Desktop_0.1.service" <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<service>\n"
        "  <provider>facebook</provider>\n"
        "  <type>a random type</type>\n"
        "</service>" <<
        "services/com.ubuntu.test_Desktop.service" <<
        "a random type";
}

void OnlineAccountsHooksTest::testServiceType()
{
    QFETCH(QString, hookName);
    QFETCH(QString, contents);
    QFETCH(QString, installedName);
    QFETCH(QString, expectedServiceType);

    writeHookFile(hookName, contents);
    QVERIFY(runHookProcess());

    // check that the file has been created, if the file was valid
    QFileInfo fileInfo(m_installDir.absoluteFilePath(installedName));
    QVERIFY(fileInfo.exists());

    QFile file(fileInfo.absoluteFilePath());
    QVERIFY(file.open(QIODevice::ReadOnly));

    // check that's a valid XML file
    QDomDocument doc;
    QVERIFY(doc.setContent(&file));

    QDomElement root = doc.documentElement();

    /* Check that a "type" element has been added and that matches the
     * expected value. */
    QDomElement serviceTypeElement = root.firstChildElement("type");
    QCOMPARE(serviceTypeElement.text(), expectedServiceType);
}

QTEST_MAIN(OnlineAccountsHooksTest);

#include "tst_online_accounts_hooks.moc"

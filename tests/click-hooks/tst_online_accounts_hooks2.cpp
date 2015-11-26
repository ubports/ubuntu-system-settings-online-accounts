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

#include <Accounts/Account>
#include <Accounts/Manager>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>
#include <SignOn/Identity>
#include <SignOn/IdentityInfo>
#include <libqtdbusmock/DBusMock.h>
#include "fake_signond.h"

#define TEST_DIR "/tmp/hooks-test2"

namespace QTest {
template<>
char *toString(const QSet<QString> &set)
{
    QByteArray ba = "QSet<QString>(";
    QStringList list = set.toList();
    ba += list.join(", ");
    ba += ")";
    return qstrdup(ba.data());
}
} // QTest namespace

// map file name -> contents
typedef QHash<QString,QString> FileData;

class OnlineAccountsHooksTest: public QObject
{
    Q_OBJECT

public:
    OnlineAccountsHooksTest();

private Q_SLOTS:
    void initTestCase();
    void cleanup() {
        if (!QTest::currentTestFailed()) {
            clearHooksDir();
            clearInstallDir();
            clearPackageDir();
        }
    }
    void testNoFiles();
    void testValidHooks_data();
    void testValidHooks();
    void testRemoval();
    void testRemovalWithAcl();
    void testTimestampRemoval();

private:
    void clearHooksDir();
    void clearInstallDir();
    void clearPackageDir();
    bool runHookProcess();
    bool runXmlDiff(const QString &generated, const QString &expected);
    void writeHookFile(const QString &name, const QString &contents);
    void writeInstalledFile(const QString &name, const QString &contents);
    void writePackageFile(const QString &name,
                          const QString &contents = QString());
    QStringList findGeneratedFiles() const;

private:
    QtDBusTest::DBusTestRunner m_dbus;
    QtDBusMock::DBusMock m_mock;
    FakeSignond m_signond;
    QByteArray m_busAddress;
    QDir m_testDir;
    QDir m_hooksDir;
    QDir m_installDir;
    QDir m_packageDir;
};

OnlineAccountsHooksTest::OnlineAccountsHooksTest():
    QObject(0),
    m_dbus(),
    m_mock(m_dbus),
    m_signond(&m_mock),
    m_testDir(TEST_DIR),
    m_hooksDir(TEST_DIR "/online-accounts-hooks2"),
    m_installDir(TEST_DIR "/accounts"),
    m_packageDir(TEST_DIR "/package")
{
    m_busAddress = qgetenv("DBUS_SESSION_BUS_ADDRESS");
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

void OnlineAccountsHooksTest::clearPackageDir()
{
    m_packageDir.removeRecursively();
    m_packageDir.mkpath(".");
}

bool OnlineAccountsHooksTest::runHookProcess()
{
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start(HOOK_PROCESS);
    if (!process.waitForFinished()) return false;

    return process.exitCode() == EXIT_SUCCESS;
}

bool OnlineAccountsHooksTest::runXmlDiff(const QString &generated,
                                         const QString &expected)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    QStringList args;
    args.append(generated);
    args.append(expected);
    process.start("xmldiff", args);
    if (!process.waitForFinished()) return false;

    int exitCode = process.exitCode();
    if (exitCode != EXIT_SUCCESS) {
        process.start("cat", args);
        process.waitForFinished();
    }
    return exitCode == EXIT_SUCCESS;
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

static QStringList findFiles(const QDir &dir)
{
    QStringList files = dir.entryList(QDir::Files);

    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    Q_FOREACH(const QString &d, dirs) {
        QStringList dirFiles = findFiles(QDir(dir.filePath(d)));
        Q_FOREACH(const QString &file, dirFiles) {
            files.append(dir.relativeFilePath(d + "/" + file));
        }
    }

    return files;
}

QStringList OnlineAccountsHooksTest::findGeneratedFiles() const
{
    return findFiles(m_installDir);
}

void OnlineAccountsHooksTest::initTestCase()
{
    qputenv("XDG_DATA_HOME", TEST_DIR);
    qputenv("OAH_CLICK_DIR", m_packageDir.path().toUtf8());
    qputenv("ACCOUNTS", TEST_DIR);
    qputenv("SSO_USE_PEER_BUS", "0");

    // The hook must be able to run without a D-Bus session
    qunsetenv("DBUS_SESSION_BUS_ADDRESS");

    clearHooksDir();
    clearInstallDir();
    clearPackageDir();
}

void OnlineAccountsHooksTest::testNoFiles()
{
    QVERIFY(runHookProcess());
    QVERIFY(m_hooksDir.entryList(QDir::Files).isEmpty());
    QVERIFY(findGeneratedFiles().isEmpty());
}

void OnlineAccountsHooksTest::testValidHooks_data()
{
    QTest::addColumn<QString>("hookName");
    QTest::addColumn<QString>("contents");
    QTest::addColumn<FileData>("packageFiles");
    QTest::addColumn<FileData>("expectedFiles");

    FileData files;
    FileData package;
    QTest::newRow("empty file") <<
        "com.ubuntu.test_MyApp_0.1.accounts" <<
        "" <<
        package <<
        files;

    QTest::newRow("empty dictionary") <<
        "com.ubuntu.test_MyApp_0.1.accounts" <<
        "{}" <<
        package <<
        files;

    files["applications/com.ubuntu.test_MyApp.application"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<application id=\"com.ubuntu.test_MyApp\">\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "  <desktop-entry>com.ubuntu.test_MyApp_0.2</desktop-entry>\n"
        "  <services>\n"
        "    <service id=\"com.ubuntu.test_MyApp_google\">\n"
        "      <description>Publish to Picasa</description>\n"
        "    </service>\n"
        "  </services>\n"
        "</application>\n";
    files["services/com.ubuntu.test_MyApp_google.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<service id=\"com.ubuntu.test_MyApp_google\">\n"
        "  <type>com.ubuntu.test_MyApp</type>\n"
        "  <provider>google</provider>\n"
        "  <name>Picasa</name>\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "  <template>\n"
        "    <setting name=\"Key\">value</setting>\n"
        "    <group name=\"a\">\n"
        "      <group name=\"b\">\n"
        "        <group name=\"c\">\n"
        "          <setting name=\"Key3\">value3</setting>\n"
        "          <setting name=\"Key4\">value4</setting>\n"
        "        </group>\n"
        "      </group>\n"
        "    </group>\n"
        "    <group name=\"other\">\n"
        "      <setting name=\"key\">value5</setting>\n"
        "    </group>\n"
        "    <group name=\"subgroup\">\n"
        "      <setting name=\"Key2\">value2</setting>\n"
        "    </group>\n"
        "  </template>\n"
        "</service>\n";
    QTest::newRow("one service") <<
        "com.ubuntu.test_MyApp_0.2.accounts" <<
        "{"
        "  \"services\": ["
        "    {"
        "      \"provider\": \"google\","
        "      \"description\": \"Publish to Picasa\","
        "      \"name\": \"Picasa\","
        "      \"settings\": {"
        "        \"Key\": \"value\","
        "        \"subgroup\": {"
        "          \"Key2\": \"value2\""
        "        },"
        "        \"a/b/c\": {"
        "          \"Key3\": \"value3\","
        "          \"Key4\": \"value4\""
        "        },"
        "        \"other/key\": \"value5\""
        "      }"
        "    }"
        "  ]"
        "}" <<
        package <<
        files;

    files["applications/com.ubuntu.test_MyApp.application"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<application id=\"com.ubuntu.test_MyApp\">\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "  <desktop-entry>com.ubuntu.test_MyApp_0.2</desktop-entry>\n"
        "  <services>\n"
        "    <service id=\"com.ubuntu.test_MyApp_google\">\n"
        "      <description>.</description>\n"
        "    </service>\n"
        "  </services>\n"
        "</application>\n";
    files["services/com.ubuntu.test_MyApp_google.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<service id=\"com.ubuntu.test_MyApp_google\">\n"
        "  <type>com.ubuntu.test_MyApp</type>\n"
        "  <provider>google</provider>\n"
        "  <name>.</name>\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "</service>\n";
    QTest::newRow("minimal") <<
        "com.ubuntu.test_MyApp_0.2.accounts" <<
        "{"
        "  \"services\": ["
        "    {"
        "      \"provider\": \"google\""
        "    }"
        "  ]"
        "}" <<
        package <<
        files;

    files.clear();
    files["applications/com.ubuntu.test_MyApp.application"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<application id=\"com.ubuntu.test_MyApp\">\n"
        "  <profile>com.ubuntu.test_MyApp_0.3</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "  <desktop-entry>com.ubuntu.test_MyApp_0.3</desktop-entry>\n"
        "  <translations>my-app</translations>\n"
        "  <services>\n"
        "    <service id=\"com.ubuntu.test_MyApp_facebook\">\n"
        "      <description>Publish to Facebook</description>\n"
        "    </service>\n"
        "    <service id=\"com.ubuntu.test_MyApp_google\">\n"
        "      <description>Publish to Picasa</description>\n"
        "    </service>\n"
        "  </services>\n"
        "</application>\n";
    files["services/com.ubuntu.test_MyApp_google.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<service id=\"com.ubuntu.test_MyApp_google\">\n"
        "  <type>com.ubuntu.test_MyApp</type>\n"
        "  <provider>google</provider>\n"
        "  <name>Picasa</name>\n"
        "  <profile>com.ubuntu.test_MyApp_0.3</profile>\n"
        "  <translations>my-app</translations>\n"
        "</service>\n";
    files["services/com.ubuntu.test_MyApp_facebook.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<service id=\"com.ubuntu.test_MyApp_facebook\">\n"
        "  <type>com.ubuntu.test_MyApp</type>\n"
        "  <provider>facebook</provider>\n"
        "  <name>Facebook photos</name>\n"
        "  <profile>com.ubuntu.test_MyApp_0.3</profile>\n"
        "  <translations>my-app</translations>\n"
        "</service>\n";
    QTest::newRow("two services") <<
        "com.ubuntu.test_MyApp_0.3.accounts" <<
        "{"
        "  \"translations\": \"my-app\","
        "  \"services\": ["
        "    {"
        "      \"provider\": \"facebook\","
        "      \"description\": \"Publish to Facebook\","
        "      \"name\": \"Facebook photos\""
        "    },"
        "    {"
        "      \"provider\": \"google\","
        "      \"description\": \"Publish to Picasa\","
        "      \"name\": \"Picasa\""
        "    }"
        "  ]"
        "}" <<
        package <<
        files;

    files.clear();
    files["applications/com.ubuntu.test_MyApp2.application"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<application id=\"com.ubuntu.test_MyApp2\">\n"
        "  <profile>com.ubuntu.test_MyApp2_0.2</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "  <desktop-entry>com.ubuntu.test_MyApp2_0.2</desktop-entry>\n"
        "  <services>\n"
        "    <service id=\"com.ubuntu.test_MyApp2_google\">\n"
        "      <description>Publish to Picasa</description>\n"
        "    </service>\n"
        "  </services>\n"
        "</application>\n";
    files["services/com.ubuntu.test_MyApp2_google.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<service id=\"com.ubuntu.test_MyApp2_google\">\n"
        "  <type>com.ubuntu.test_MyApp2</type>\n"
        "  <provider>google</provider>\n"
        "  <name>Picasa</name>\n"
        "  <profile>com.ubuntu.test_MyApp2_0.2</profile>\n"
        "  <template>\n"
        "    <group name=\"auth\">\n"
        "      <setting name=\"method\">oauth2</setting>\n"
        "      <setting name=\"mechanism\">web_server</setting>\n"
        "      <group name=\"oauth2\">\n"
        "        <group name=\"web_server\">\n"
        "          <setting name=\"ClientId\">foo</setting>\n"
        "          <setting name=\"ClientSecret\">bar</setting>\n"
        "          <setting name=\"Scopes\" type=\"as\">['one scope','and another']</setting>\n"
        "          <setting name=\"UseSSL\" type=\"b\">false</setting>\n"
        "        </group>\n"
        "      </group>\n"
        "    </group>\n"
        "  </template>\n"
        "</service>\n";
    QTest::newRow("one service, with auth data") <<
        "com.ubuntu.test_MyApp2_0.2.accounts" <<
        "{"
        "  \"services\": ["
        "    {"
        "      \"provider\": \"google\","
        "      \"description\": \"Publish to Picasa\","
        "      \"name\": \"Picasa\","
        "      \"auth\": {"
        "        \"oauth2/web_server\": {"
        "          \"ClientId\": \"foo\","
        "          \"ClientSecret\": \"bar\","
        "          \"UseSSL\": false,"
        "          \"Scopes\": [\"one scope\",\"and another\"]"
        "        }"
        "      }"
        "    }"
        "  ]"
        "}" <<
        package <<
        files;

    package["myapp/Main.qml"] = "Something here";
    package["google.svg"] = "icon contents";
    files.clear();
    files["applications/com.ubuntu.test_MyApp.application"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<application id=\"com.ubuntu.test_MyApp\">\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "  <desktop-entry>com.ubuntu.test_MyApp_0.2</desktop-entry>\n"
        "  <services>\n"
        "    <service id=\"com.ubuntu.test_MyApp_google\">\n"
        "      <description>.</description>\n"
        "    </service>\n"
        "  </services>\n"
        "</application>\n";
    files["services/com.ubuntu.test_MyApp_google.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<service id=\"com.ubuntu.test_MyApp_google\">\n"
        "  <type>com.ubuntu.test_MyApp</type>\n"
        "  <provider>google</provider>\n"
        "  <name>.</name>\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "</service>\n";
    files["providers/com.ubuntu.test_MyApp_google.provider"] =
        QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<provider id=\"com.ubuntu.test_MyApp_google\">\n"
        "  <name>Google</name>\n"
        "  <icon>%1/google.svg</icon>\n"
        "  <profile>com.ubuntu.test_MyApp_0.2</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "</provider>\n").arg(m_packageDir.path());
    files["qml-plugins/com.ubuntu.test_MyApp_google/Main.qml"] =
        "Something here";
    QTest::newRow("account plugin, icon file") <<
        "com.ubuntu.test_MyApp_0.2.accounts" <<
        "{"
        "  \"services\": ["
        "    {"
        "      \"provider\": \"google\""
        "    }"
        "  ],"
        "  \"plugins\": ["
        "    {"
        "      \"provider\": \"google\","
        "      \"name\": \"Google\","
        "      \"icon\": \"google.svg\","
        "      \"qml\": \"myapp\""
        "    }"
        "  ]"
        "}" <<
        package <<
        files;

    files.clear();
    package.clear();
    package["myapp/Main.qml"] = "Something herez";
    files["applications/com.ubuntu.test_MyApp.application"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<application id=\"com.ubuntu.test_MyApp\">\n"
        "  <profile>com.ubuntu.test_MyApp_0.3</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "  <desktop-entry>com.ubuntu.test_MyApp_0.3</desktop-entry>\n"
        "  <services>\n"
        "    <service id=\"com.ubuntu.test_MyApp_abc\">\n"
        "      <description>.</description>\n"
        "    </service>\n"
        "  </services>\n"
        "</application>\n";
    files["services/com.ubuntu.test_MyApp_abc.service"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<service id=\"com.ubuntu.test_MyApp_abc\">\n"
        "  <type>com.ubuntu.test_MyApp</type>\n"
        "  <provider>abc</provider>\n"
        "  <name>.</name>\n"
        "  <profile>com.ubuntu.test_MyApp_0.3</profile>\n"
        "</service>\n";
    files["providers/com.ubuntu.test_MyApp_abc.provider"] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<provider id=\"com.ubuntu.test_MyApp_abc\">\n"
        "  <name>A B C</name>\n"
        "  <icon>abc.svg</icon>\n"
        "  <profile>com.ubuntu.test_MyApp_0.3</profile>\n"
        "  <package-dir>/tmp/hooks-test2/package</package-dir>\n"
        "</provider>\n";
    files["qml-plugins/com.ubuntu.test_MyApp_abc/Main.qml"] =
        "Something herez";
    QTest::newRow("account plugin, stock icon") <<
        "com.ubuntu.test_MyApp_0.3.accounts" <<
        "{"
        "  \"services\": ["
        "    {"
        "      \"provider\": \"abc\""
        "    }"
        "  ],"
        "  \"plugins\": ["
        "    {"
        "      \"provider\": \"abc\","
        "      \"name\": \"A B C\","
        "      \"icon\": \"abc.svg\","
        "      \"qml\": \"myapp\""
        "    }"
        "  ]"
        "}" <<
        package <<
        files;
}

void OnlineAccountsHooksTest::testValidHooks()
{
    QFETCH(QString, hookName);
    QFETCH(QString, contents);
    QFETCH(FileData, packageFiles);
    QFETCH(FileData, expectedFiles);

    writeHookFile(hookName, contents);
    for (FileData::const_iterator i = packageFiles.constBegin();
         i != packageFiles.constEnd(); i++) {
        writePackageFile(i.key(), i.value());
    }
    QVERIFY(runHookProcess());

    QCOMPARE(findGeneratedFiles().toSet(), expectedFiles.keys().toSet());

    QStringList xmlSuffixes;
    xmlSuffixes << "provider" << "service" << "application";
    for (FileData::const_iterator i = expectedFiles.constBegin();
         i != expectedFiles.constEnd(); i++) {
        QFileInfo generatedFile(m_installDir.filePath(i.key()));
        if (xmlSuffixes.contains(generatedFile.suffix())) {
            /* Dump the expected XML into a file */
            QString expectedXml = generatedFile.filePath() + ".expected";
            QFile file(expectedXml);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QByteArray expectedContents = i.value().toUtf8();
            QCOMPARE(file.write(expectedContents), expectedContents.length());
            file.close();

            QVERIFY(runXmlDiff(generatedFile.filePath(), expectedXml));
        } else {
            // direct comparison
            QFile file(generatedFile.filePath());
            QVERIFY(file.open(QIODevice::ReadOnly));
            QByteArray expectedContents = i.value().toUtf8();
            QCOMPARE(file.readAll(), expectedContents);
        }
    }
}

void OnlineAccountsHooksTest::testRemoval()
{
    QString stillInstalled("applications/com.ubuntu.test_StillInstalled.application");
    writeInstalledFile(stillInstalled,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
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
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <services>\n"
        "    <service id=\"com-ubuntu.test_MyApp_example\">\n"
        "      <description>Publish somewhere</description>\n"
        "    </service>\n"
        "  </services>\n"
        "  <profile>com-ubuntu.test_MyApp_3.0</profile>\n"
        "</application>");
    QVERIFY(m_installDir.exists(myApp));

    QString myService("services/com.ubuntu.test_MyApp_example.service");
    writeInstalledFile(myService,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<service>\n"
        "  <name>Hello world</name>\n"
        "  <provider>example</provider>\n"
        "  <description>My application</description>\n"
        "  <profile>com-ubuntu.test_MyApp_3.0</profile>\n"
        "</service>");
    QVERIFY(m_installDir.exists(myService));

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

    QString notOurs("applications/com.ubuntu.test_NotOurs.application");
    writeInstalledFile(notOurs,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<!--this file is auto-generated by online-accounts-hooks; do not modify-->\n"
        "<application>\n"
        "  <description>My application</description>\n"
        "  <service-types>\n"
        "    <service-type>some type</service-type>\n"
        "  </service-types>\n"
        "  <profile>com-ubuntu.test_NotOurs_0.1.0</profile>\n"
        "</application>");
    QVERIFY(m_installDir.exists(notOurs));

    writeHookFile("com-ubuntu.test_StillInstalled_2.0.accounts",
        "{"
        "  \"services\": ["
        "    {"
        "      \"provider\": \"example\","
        "      \"description\": \"Publish somewhere\","
        "      \"name\": \"Hello world\""
        "    }"
        "  ]"
        "}");

    QVERIFY(runHookProcess());

    QVERIFY(m_installDir.exists(stillInstalled));
    QVERIFY(!m_installDir.exists(myApp));
    QVERIFY(!m_installDir.exists(myService));
    QVERIFY(m_installDir.exists(noProfile));
    QVERIFY(m_installDir.exists(notOurs));
}

void OnlineAccountsHooksTest::testRemovalWithAcl()
{
    qputenv("DBUS_SESSION_BUS_ADDRESS", m_busAddress);
    m_dbus.startServices();

    QString myApp("applications/com.ubuntu.test_MyAcl.application");
    writeInstalledFile(myApp,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<application id=\"com-ubuntu.test_MyAcl\">\n"
        "  <description>My application</description>\n"
        "  <services>\n"
        "    <service id=\"com-ubuntu.test_MyAcl_example\">\n"
        "      <description>Publish somewhere</description>\n"
        "    </service>\n"
        "  </services>\n"
        "  <profile>com-ubuntu.test_MyAcl_3.0</profile>\n"
        "</application>");
    QVERIFY(m_installDir.exists(myApp));

    QString myService("services/com.ubuntu.test_MyAcl_example.service");
    writeInstalledFile(myService,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<!--this file is auto-generated by online-accounts-hooks2; do not modify-->\n"
        "<service id=\"com-ubuntu.test_MyAcl_example\">\n"
        "  <name>Hello world</name>\n"
        "  <type>com-ubuntu.test_MyAcl</type>\n"
        "  <provider>example</provider>\n"
        "  <description>My application</description>\n"
        "  <profile>com-ubuntu.test_MyAcl_3.0</profile>\n"
        "</service>");
    QVERIFY(m_installDir.exists(myService));

    /* Create an account, enable the app and add it to the ACL */
    Accounts::Manager manager;
    Accounts::Service service =
        manager.service("com.ubuntu.test_MyAcl_example");
    QVERIFY(service.isValid());
    Accounts::Account *account = manager.createAccount("example");
    account->setDisplayName("Example account");
    account->setEnabled(true);
    account->setCredentialsId(25);
    account->selectService(service);
    account->setEnabled(true);
    account->syncAndBlock();
    Accounts::AccountId accountId = account->id();
    QVERIFY(accountId > 0);

    QVariantMap initialInfo;
    QStringList initialAcl;
    initialAcl << "one" << "com-ubuntu.test_MyAcl_0.1" << "two_click";
    initialInfo["ACL"] = initialAcl;
    initialInfo["Id"] = 25;
    m_signond.addIdentity(25, initialInfo);

    /* Now run the hook process; it should delete the .service and .application
     * files, and also disable the service and remove the app from the ACL */
    QVERIFY(runHookProcess());

    QVERIFY(!m_installDir.exists(myApp));
    QVERIFY(!m_installDir.exists(myService));
    QTRY_COMPARE(account->isEnabled(), false);

    SignOn::Identity *identity = SignOn::Identity::existingIdentity(25, this);
    QSignalSpy gotInfo(identity, SIGNAL(info(const SignOn::IdentityInfo&)));
    identity->queryInfo();
    QTRY_COMPARE(gotInfo.count(), 1);

    SignOn::IdentityInfo info =
        gotInfo.at(0).at(0).value<SignOn::IdentityInfo>();
    QStringList expectedAcl;
    expectedAcl << "one" << "two_click";
    QCOMPARE(info.accessControlList().toSet(), expectedAcl.toSet());
}

void OnlineAccountsHooksTest::testTimestampRemoval()
{
    QString stillInstalled("com-ubuntu.test_MyApp_2.0.accounts");
    writeHookFile(stillInstalled, "{}");
    QString stillInstalledTimestamp("com-ubuntu.test_MyApp_2.0.accounts.processed");
    writeHookFile(stillInstalledTimestamp, "");
    QString oldTimestamp("com-ubuntu.test_MyApp_1.0.accounts.processed");
    writeHookFile(oldTimestamp, "");
    QString staleTimestamp1("com-ubuntu.app_MyOtherApp_2.0.accounts.processed");
    writeHookFile(staleTimestamp1, "");
    QString staleTimestamp2("com-ubuntu.app_MyOtherApp_1.0.accounts.processed");
    writeHookFile(staleTimestamp2, "");

    QVERIFY(runHookProcess());

    QVERIFY(m_hooksDir.exists(stillInstalled));
    QVERIFY(m_hooksDir.exists(stillInstalledTimestamp));
    QVERIFY(!m_hooksDir.exists(oldTimestamp));
    QVERIFY(!m_hooksDir.exists(staleTimestamp1));
    QVERIFY(!m_hooksDir.exists(staleTimestamp2));
}

QTEST_GUILESS_MAIN(OnlineAccountsHooksTest);

#include "tst_online_accounts_hooks2.moc"
